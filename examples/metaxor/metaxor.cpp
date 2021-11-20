#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <conio.h>

#include <rtctimer.h>
#include <dpmi.h>
#include <vga.h>

#define min(a, b) (((a) > (b)) ? (b) : (a))
#define max(a, b) (((a) < (b)) ? (b) : (a))

const float PI = 3.141592653589793;

enum {
    X_RES   = 640,
    X_CHARS = (X_RES >> 3),
    Y_RES   = 350,
    
    VIDEO_PAGES         = 2,
    VIDEO_PAGE_SIZE     = (0x10000 / VIDEO_PAGES),
    SPRITES_COUNT       = 2,
    
    SPRITE_X_RES_LOG2   = 10,                            // log2(n), 10 = 1024 pixels = 128 chars
    SPRITE_X_CHARS_LOG2 = SPRITE_X_RES_LOG2 - 3,
    SPRITE_X_RES        = (1 << SPRITE_X_RES_LOG2),
    SPRITE_X_CHARS      = (1 << SPRITE_X_CHARS_LOG2),
    SPRITE_Y_RES        = 768,
    
    SPRITE_RING_SIZE_LOG2 = 4,
    SPRITE_RING_SIZE   = (1 <<  SPRITE_RING_SIZE_LOG2),
    SPRITE_RING_SIZE2X = (1 << (SPRITE_RING_SIZE_LOG2 + 1)),
};

// xor sprite - 8 preshifted copies
uint8_t sprite[(SPRITE_X_CHARS * 8) * SPRITE_Y_RES];

// offset tables 
uint32_t spriteOffsets[SPRITES_COUNT][Y_RES];


void makeTable() {
    
    // make main sprite
    uint8_t *p = sprite;
    for (int32_t y = -SPRITE_Y_RES/2; y < SPRITE_Y_RES/2; y++) {
        for (int32_t x = -SPRITE_X_RES/2; x < SPRITE_X_RES/2; x += 8) {
            // accumulate pixels in one byte
            uint8_t buffer = 0;
            for (int32_t sx = 0; sx < 8; sx++) {
                // shift already stored pixels
                buffer <<= 1;
                // and put new pixel to MSB
                buffer |= (((int32_t)sqrt((x+sx)*(x+sx) + y*y)) & (SPRITE_RING_SIZE2X-1)) >> SPRITE_RING_SIZE_LOG2;
            }
            // store the pixel
            *p++ = buffer;
        }
        
        // skip shifted copies
        p += SPRITE_X_CHARS * 7;
    }
    
    
    // preshift
    for (int32_t k = 1; k < 8; k++) {
        
        // get copy to preshift
        p = sprite;
        uint8_t *v = sprite + (SPRITE_X_CHARS * k);
        
        // do preshift
        for (size_t y = 0; y < SPRITE_Y_RES; y++) {
            for (size_t x = 0; x < SPRITE_X_CHARS; x++) {
                *v++ = (*p << k) | (*(p+1) >> (8 - k)); p++;
            }
            // skip shifted copies
            p += SPRITE_X_CHARS * 7;
            v += SPRITE_X_CHARS * 7;
        }
    }
    
}

void calcScreen(uint32_t frameCounter) {
    for (uint32_t s = 0; s < SPRITES_COUNT; s++) {
        float ofsX = 0.4*((SPRITE_X_RES-X_RES) * (sin(0.0051*0.2*frameCounter * (1.24+0.38*s)) + 1.2));
        float ofsY = 0.5*((SPRITE_Y_RES-Y_RES) * (cos(0.0066*0.2*frameCounter * (1.23+0.25*s)) + 1.));
        for (uint32_t y = 0; y < Y_RES; y++) {
            float sinX = 12*sin(0.2*sin(0.09*y+0.001*frameCounter)+0.02*0.03*frameCounter+0.2*s);
            float sinY = 0;
            
            int iofsX = (int)(ofsX + sinX) & (SPRITE_X_RES-1);
            int iofsY = (int)(ofsY + sinY);
            int shift = (iofsX & 7) << SPRITE_X_CHARS_LOG2;
            
            spriteOffsets[s][y] = (iofsY << SPRITE_X_RES_LOG2) + shift + (iofsX >> 3);
            
            ofsY += 1.;
        }
    }
}

void renderScreen(uint32_t frameCounter) {       
    uint32_t currentPage = frameCounter % VIDEO_PAGES;
    uint32_t pageOffset  = VIDEO_PAGE_SIZE*currentPage;
    
    // select plane mask register (0x3C4 index 2)
    outp(VGA_SEQ_INDEX, VGA_SEQ_PLANE_WRITE_MASK);
    
    for (size_t s = 0; s < SPRITES_COUNT; s++) {
        // restore screen write pointer
        uint8_t *screen = (uint8_t*)(0xA0000 + VIDEO_PAGE_SIZE*currentPage);
        
        // set bitplane to write
        outp(VGA_SEQ_DATA, (1 << s));
        
        for (size_t y = 0; y < Y_RES; y++) {
            memcpy(screen, sprite + spriteOffsets[s][y], X_CHARS);
            screen += X_CHARS;
        }
    }
        
    // video page select
    outpw(VGA_CRTC_INDEX, ((pageOffset >> 8  ) << 8) | VGA_CRTC_START_ADDRESS_HIGH);
    outpw(VGA_CRTC_INDEX, ((pageOffset & 0xFF) << 8) | VGA_CRTC_START_ADDRESS_LOW);
}

#define docga(i, r, g, b) ( \
    (((b) & 1) << 0) |  \
    (((g) & 1) << 1) |  \
    (((r) & 1) << 2) |  \
    (((i) & 1) << 4)    \
    )

#define doega(r, g, b) ( \
    (((b) & 2) >> 1) |  \
    (((g) & 2) << 1) |  \
    (((r) & 2) << 2) |  \
    (((b) & 1) << 3) |  \
    (((g) & 1) << 4) |  \
    (((r) & 1) << 5)    \
    )

void makePalette() {
    // reset attribute flip-flop
    inp(VGA_INPUT_STATUS);
    
#if 1
    // set palette
    uint8_t pal[] = {
        doega(0, 0, 1),
        doega(0, 0, 2),
        doega(0, 0, 2),
        doega(0, 0, 1),
        
        doega(0, 1, 1),
        doega(0, 1, 2),
        doega(0, 1, 2),
        doega(0, 1, 1),
        
        doega(0, 1, 1),
        doega(0, 1, 2),
        doega(0, 1, 2),
        doega(0, 1, 1),
        
        doega(0, 0, 1),
        doega(0, 0, 2),
        doega(0, 0, 2),
        doega(0, 0, 1),
    };
#else
    uint8_t pal[] = {
        docga(0, 0, 0, 1),
        docga(1, 0, 0, 1),
        docga(1, 0, 0, 1),
        docga(0, 0, 0, 1),
    };
#endif
    
    // set palette for planes
    for (size_t i = 0; i < sizeof(pal); i++) {
        outp(VGA_ATC_INDEX_DATA, VGA_ATC_PALETTE + i);
        outp(VGA_ATC_INDEX_DATA, pal[i]);
    }
    
    // enable palette source
    outp(VGA_ATC_INDEX_DATA, VGA_ATC_PAL_SOURCE);
}


// and finally, the main loop

int main() {
    makeTable();
    rtc_initTimer(RTC_TIMER_RATE_1024HZ);
    vgaInit();
    vgaSetMode(0x10);
    makePalette();
    
    size_t frameCounter = 0, tick = 0;
    while (!kbhit()) {
        vgaVsync();
        renderScreen(frameCounter);
        calcScreen(tick);
        
        tick = rtc_getTick();
        frameCounter++;
    }; getch();
    
    vgaSetMode(0x3);
    vgaDone();
    rtc_freeTimer();
}