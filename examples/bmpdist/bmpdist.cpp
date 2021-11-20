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

#define min(a, b) (((a) > (b)) ? (b) : (a))
#define max(a, b) (((a) < (b)) ? (b) : (a))

const float PI = 3.141592653589793;

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

grid grid[(Y_GRID+1)*(X_GRID+1)];

void renderScreen(uint32_t frameCounter) {   
    // interpolation sturct
    typedef struct {int sdy, edy, dx, sy, ey, sx;} _fd;
    
    size_t gridptr = 0;
    uint8_t *p = screen;
    uint8_t *t = texture;
    _fd u, v;
    
    // for asm inner loop
    static int xu, xv, xdu, xdv;
    
    for (int j = 0; j < Y_GRID; j++) {
        for (int i = 0; i < X_GRID; i++) {
            
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
                
                
                _asm {
                    xor     eax, eax
                    mov     ebx, xu
                    mov     edx, xv
                    mov     ecx, 8
                    mov     esi, t
                    mov     edi, p
                    
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
                
                
                p += X_RES;
                u.sy += u.sdy; u.ey += u.edy; v.sy += v.sdy; v.ey += v.edy;
            }
            gridptr++;
            p -= (X_RES * 8) - 8;
        }
        gridptr++;
        p += (X_RES * 7);
    } 
}

void makeGrid(uint32_t frameCounter) {

    size_t tptr = 0;
    
    int32_t u, v;
    float fu, fv;

    float ax, ay;
    
    int32_t t = (frameCounter * 60) / 1024;
    
    for (int32_t y = -Y_GRID/2; y <= Y_GRID/2; y++) {
        for (int32_t x = -X_GRID/2; x <= X_GRID/2; x++) {
            
            ax = (x * 8) + 64*sintabf[((t << 7) + (x << 8)) & 0xFFFF] +
                  (32*sintabf[((y << 7) + (t << 6)) & 0xFFFF] + 32) * 
                   sintabf[((y << 6) + (x << 9) + (t << 4)) & 0xFFFF] + 
                   64*sintabf[((t << 8) + (x << 10)) & 0xFFFF] +
                   32*sintabf[((t << 8) + (y << 10)) & 0xFFFF]; 
            ay = (y * 8) + 64*sintabf[((t << 7) + (y << 7)) & 0xFFFF] +
                  (32*sintabf[((x << 6) + (t << 6)) & 0xFFFF] + 32) * 
                   sintabf[((x << 6) + (y << 8) + (t << 4)) & 0xFFFF] + 
                   64*sintabf[((t << 8) + (y << 10)) & 0xFFFF] +
                   32*sintabf[((t << 7) + (x << 9)) & 0xFFFF]; 
            
            u = (int32_t)(ax * 256);
            v = (int32_t)(ay * 256);
            
            grid[tptr].u = u;// & 0xFFFFFFFF;
            grid[tptr].v = v;// & 0xFFFFFFFF;
            
            tptr++;
        }
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
    initSintab();
    makeTexture("dutch8.bmp");
    rtc_initTimer(RTC_TIMER_RATE_1024HZ);
    vgaInit();
    vgaSetMode(0x13);
    makePalette();
    
    size_t frameCounter = 0, tick = 0;
    while (!kbhit()) {
        vgaVsync();
        vgaMemcpyA000(screen, 0, X_RES*Y_RES);
        
        makeGrid(tick);
        renderScreen(tick);
        
        tick = rtc_getTick();
        frameCounter++;
    }; getch();
    
    vgaSetMode(0x3);
    vgaDone();
    rtc_freeTimer();
}