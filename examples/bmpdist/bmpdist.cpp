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

#include "vec.h"

enum {
    X_RES = 320,
    Y_RES = 200,
    
    GRID_SIZE = 8,
    X_GRID = X_RES / GRID_SIZE,
    Y_GRID = Y_RES / GRID_SIZE,
    
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

// sine/cosine tables
signed int sintab[65536];
float      sintabf[65536];

void initSintab() {
    int i, j;
    float r;
    
    for (i = 0; i < 65536; i++) {
        r = (sin(2 * PI * i / 65536));
        sintab[i] = 32767 * r;
        sintabf[i] = r;
    }
}

// grid struct
struct grid {
    int32_t u, v;
};

// grid table
grid grid[(Y_GRID+1)*(X_GRID+1)];

// map texture to screen using calculated U/V grid
void renderScreen(uint32_t frameCounter) {   
    // interpolation sturct
    typedef struct {int sdy, edy, dx, sy, ey, sx;} _fd;
    
    // pointers and other stuff
    size_t gridptr = 0;
    uint8_t *p = screen;
    uint8_t *t = texture;
    _fd u, v;
    
    // for asm inner loop
    static int xu, xv, xdu, xdv;
    
    for (int j = 0; j < Y_GRID; j++) {
        for (int i = 0; i < X_GRID; i++) {
            
            // prepare for U/V interpolation across each 8x8 cell
            u.sdy = (grid[gridptr+X_GRID+1].u - grid[gridptr].u) >> 3;
            u.sy  = (grid[gridptr].u);
            
            u.edy = (grid[gridptr+X_GRID+2].u - grid[gridptr+1].u) >> 3;
            u.ey  = (grid[gridptr+1].u);
            
            v.sdy = (grid[gridptr+X_GRID+1].v - grid[gridptr].v) >> 3;
            v.sy  = (grid[gridptr].v);
        
            v.edy = (grid[gridptr+X_GRID+2].v - grid[gridptr+1].v) >> 3;
            v.ey  = (grid[gridptr+1].v);
            
            for (int y = 0; y < 8; y++) {
                
                u.dx = (u.ey - u.sy) >> 3;
                u.sx = u.sy;
                
                v.dx = (v.ey - v.sy) >> 3;
                v.sx = v.sy;
                
                xu = u.sy;
                xv = v.sy;
                
                xdu = u.dx;
                xdv = v.dx;
                
                // assembly texture mapping innerloop
                    
                _asm {
                    xor     eax, eax
                    mov     ebx, xu
                    mov     edx, xv
                    mov     ecx, 8
                    mov     esi, t
                    mov     edi, p
                    
                    // finally, the inner loop
                    // still there is enough room for optimization (i.e. unrolling or writing 4 pixels at once),
                    // but that's left as exercise :)
                    
                    _loop:
                    mov     ah, bh              // 1
                    inc     edi                 // .
                    
                    mov     al, dh              // 2
                    add     ebx, xdu            // .
                    
                    mov     al, [esi + eax]     // 3
                    add     edx, xdv            // .
                                        
                    mov     [edi - 1], al       // 4
                    dec     ecx                 // .
                    
                    jnz     _loop               // 5
                }
                
                // advance 
                p += X_RES;
                u.sy += u.sdy; u.ey += u.edy; v.sy += v.sdy; v.ey += v.edy;
            }
            // advance grid and "rewind" to top of screen
            gridptr++;
            p -= (X_RES * 8) - 8;
        }
        // compensate for grid boundary, advance destination
        gridptr++;
        p += (X_RES * 7);
    } 
}

// build U/V grid
void makeGrid(uint32_t frameCounter) {

    size_t tptr = 0;
    
    int32_t u, v;
    float fu, fv;

    float ax, ay;
    
    // rescale time base
    int32_t t = (frameCounter * 60) / 1024;
    
    // trace the grid, including boundary column/row
    for (int32_t y = -Y_RES/2; y <= Y_RES/2; y += 8) {
        for (int32_t x = -X_RES/2; x <= X_RES/2; x += 8) {
            
            ax = x + 64*sintabf[((t << 7) + (x << 5)) & 0xFFFF] +
                  (32*sintabf[((y << 4) + (t << 6)) & 0xFFFF] + 32) * 
                   sintabf[((y << 6) + (x << 7) + (t << 4)) & 0xFFFF] + 
                   64*sintabf[((t << 8) + (x << 8)) & 0xFFFF] +
                   32*sintabf[((t << 8) + (y << 8) + 0x4000) & 0xFFFF]; 
            ay = y + 64*sintabf[((t << 7) + (y << 4)) & 0xFFFF] +
                  (32*sintabf[((x << 6) + (t << 6)) & 0xFFFF] + 32) * 
                   sintabf[((x << 6) + (y << 6) + (t << 4)) & 0xFFFF] + 
                   64*sintabf[((t << 8) + (y << 7)) & 0xFFFF] +
                   32*sintabf[((t << 7) + (x << 6) + 0x4000) & 0xFFFF]; 
            
            u = (int32_t)(ax * 256);        // rescale to 24.8 fixedpoint
            v = (int32_t)(ay * 256);        // rescale to 24.8 fixedpoint
            
            grid[tptr].u = u;// & 0xFFFFFFFF;
            grid[tptr].v = v;// & 0xFFFFFFFF;
            
            tptr++;
        }
    }
}

// load texture from file
void makeTexture(const char *filename) {
    
    BITMAPV5HEADER bmphead;
    bmp_load8_header(filename, &bmphead);
    
    if ((bmphead.bV5Width != TEXTURE_RES) || (bmphead.bV5Height != TEXTURE_RES)) return;
    
    uint32_t bmppal[256];
    bmp_load8_data("dutch8.bmp", &bmphead, texture, bmppal, -1);
    
    // convert palette from 8 bit to 6 bit
    for (size_t i = 0; i < 256; i++) texturePal[i].val = (bmppal[i] >> 2) & 0x3F3F3F;
}


// set texture palette
void makePalette() {
    vgaSetPalette(texturePal, 0, 256);
}


// and finally, the main loop
int main(int argc, char *argv[]) {
    size_t frameCounter = 0, tick = 0;
    
    if ((argc >= 2) && (strstr(argv[1], "?") != 0)) {
        printf("usage: bmpdist.exe [texture.bmp] [NOVSYNC]\n");
        return 0;
    }
    char *filename  = ((argc >= 2) ? argv[1] : "dutch8.bmp");
    bool  noVsync   = ((argc >= 3) && (strstr(strupr(argv[2]), "NOVSYNC") != 0));
    
    // init
    initSintab();
    makeTexture(filename);
    rtc_initTimer(RTC_TIMER_RATE_1024HZ);
    vgaInit();
    vgaSetMode(0x13);
    makePalette();
    
    // main loop
    while (!kbhit()) {
        if (!noVsync) vgaVsync();
        vgaMemcpyA000(screen, 0, X_RES*Y_RES);
        
        makeGrid(tick);
        renderScreen(tick);
        
        tick = rtc_getTick();
        frameCounter++;
    }; getch();
    
    vgaSetMode(0x3);
    vgaDone();
    rtc_freeTimer();
    
    // print timing results
    {
        uint32_t num    = ((frameCounter * 1024) / tick);
        uint32_t denom  = ((frameCounter * 1024 * 1000) / tick) - (num * 1000);
        printf("%u RTC ticks in %u frames - %d.%d fps", tick, frameCounter, num, denom);
    }
}