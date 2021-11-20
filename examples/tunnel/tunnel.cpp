#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <conio.h>

#include <rtctimer.h>
#include <dpmi.h>
#include <vga.h>
#include <bmpload.h>
#include <tgaload.h>

#define min(a, b) (((a) > (b)) ? (b) : (a))
#define max(a, b) (((a) < (b)) ? (b) : (a))

const float PI = 3.141592653589793;

enum {
    X_RES = 320,
    Y_RES = 200,
    
    // texture sizes for both U and V, always power of two!
    TEXTURE_RES_LOG2 = 8,                               // n
    TEXTURE_RES   = (1 << TEXTURE_RES_LOG2),            // 2^n
    TEXTURE_SIZE  = TEXTURE_RES*TEXTURE_RES,            // 2^(2n)
    TEXTURE_MASK  = TEXTURE_SIZE - 1,                   // 2^(2n)-1 - texture mask
};

// mapping table
uint16_t mappingTable[X_RES * Y_RES];

// texture and its palette
uint8_t     texture[TEXTURE_SIZE];
vgaPalColor texturePal[256];

// virtual screen buffer
uint8_t screen[X_RES * Y_RES];

void makeTable() {
    // parameter consts
    const float tunnelSize = TEXTURE_RES * 30.0;
    const float angleScale = 1.0;
    
    uint16_t *p = mappingTable;
    
    // iterate for each pixel
    for (int32_t y = -Y_RES/2; y < Y_RES/2; y++) {
        for (int32_t x = -X_RES/2; x < X_RES/2; x++) {
            
            // use the following equations for tunnel:
            // u = tunnelSize/sqrt(x*x + y*y)
            // v = atan2(y, x)
            // simplier than ever :)
            
            // calculate radius and angle first, and normalize to [1; 1] range
            float r = (sqrt(x*x + y*y) + 1e-6);
            float a = (angleScale * (atan2(y, x) + PI)) / PI;
            
            // then calculate U and V factors
            float u = tunnelSize/r;
            float v = TEXTURE_RES * (a + 0.0*sin(0.1*r));
            
            // and composite texture coordinates to mapping table
            *p++ = ((((int)u) & (TEXTURE_RES - 1)) << TEXTURE_RES_LOG2) | 
                   ((((int)v) & (TEXTURE_RES - 1)) << 0);
        }
    }
}

void renderScreen(uint32_t frameCounter) {   
    // calculate offset
    float ofsU = frameCounter*0.1;
    float ofsV = 512*sin(0.004*0.1*frameCounter)*cos(0.1*frameCounter*0.006)*sin(0.1*frameCounter*0.005);
    uint16_t offset =
        (((uint32_t)ofsU & (TEXTURE_RES - 1)) << TEXTURE_RES_LOG2) | 
          (uint32_t)ofsV & (TEXTURE_RES - 1);
    
    // map table to screen
    uint8_t  *scr = screen;
    uint16_t *map = mappingTable;
    for (size_t i = 0; i < X_RES*Y_RES; i++) {
        *scr++ = texture[(*map++ + offset) & TEXTURE_MASK];
    }
}

void makeTexture(const char *filename) {
    
    BITMAPV5HEADER bmphead;
    bmp_load8_header(filename, &bmphead);
    
    if ((bmphead.bV5Width != TEXTURE_RES) || (bmphead.bV5Height != TEXTURE_RES)) return;
    
    uint32_t bmppal[256];
    bmp_load8_data("dutch8.bmp", &bmphead, texture, bmppal, -1);
    
    // convert palette from 8 bit to 6 bit
    for (size_t i = 0; i < 256; i++) texturePal[i].val = (bmppal[i] >> 2) & 0x3F3F3F;
    
    /*
    uint8_t *p = texture;
    
    const float ee = 1e-4;
    
    // simple x ^ y pattern
    for (int32_t y = -TEXTURE_RES/2; y < TEXTURE_RES/2; y++) {
        for (int32_t x = -TEXTURE_RES/2; x < TEXTURE_RES/2; x++) {
            *p++ = max(min(((int)(0x20000 / ((x*x ^ y*y) + ee))), 255), 0);
        }
    }
    */
}


void makePalette() {
    // init palette
    //vgaPalColor pal[256];
    //for (size_t i = 0; i < 256; i++) pal[i].val = (i >> 2) * 0x010101;
    
    // and set it
    vgaSetPalette(texturePal, 0, 256);
}

// and finally, the main loop

int main() {
    makeTable();
    makeTexture("dutch8.bmp");
    rtc_initTimer(RTC_TIMER_RATE_1024HZ);
    vgaInit();
    vgaSetMode(0x13);
    makePalette();
    
    size_t frameCounter = 0;
    while (!kbhit()) {
        vgaVsync();
        vgaMemcpyA000(screen, 0, X_RES*Y_RES);
        renderScreen(frameCounter);
        frameCounter = rtc_getTick();
        //frameCounter++;
    }; getch();
    
    vgaSetMode(0x3);
    vgaDone();
    rtc_freeTimer();
}