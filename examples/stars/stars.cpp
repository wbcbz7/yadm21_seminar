#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <conio.h>

#include <rtctimer.h>
#include <dpmi.h>
#include <vga.h>

#include <fxmath.h>
#include <vec.h>


enum {
    X_RES = 320,
    Y_RES = 200,
    
    STARS_COUNT   = 8192,
    STARS_X_DISP  = 16384,
    STARS_Y_DISP  = 16384,
    STARS_Z_DISP  = 32767,
    
    // for 16.16 fixedpoint
    STARS_Z_NEAR_PLANE  = 4   * 65536,
    STARS_Z_PROJ_FOV    = 120,
    STARS_Z_ATTENUATION = 480000,
    STARS_Z_DELTA       = 84000,
    
    SINTAB_SIZE         = 1024,
    COSTAB_OFFSET       = (SINTAB_SIZE >> 2),
};

// sine table
int32_t sintab[SINTAB_SIZE]; 

// virtual screen buffer
uint8_t screen[X_RES * Y_RES];

// stars positions
vec3x starsPos[STARS_COUNT];

// xorshift seed
uint32_t xorshift32_seed = 0x555AAA55;

inline uint32_t xorshift32() {
    uint32_t x = xorshift32_seed;
    x ^= x >> 2;  // a
	x ^= x << 9;  // b
	x ^= x >> 15;  // c
    xorshift32_seed = x;
    return x;
}

// star generation
vec3x generateStar() {
    vec3x star = {
        (int32_t)(xorshift32() % (STARS_X_DISP << 17)) - (STARS_X_DISP << 16),
        (int32_t)(xorshift32() % (STARS_Y_DISP << 17)) - (STARS_Y_DISP << 16),
        (int32_t)(xorshift32() % (STARS_Z_DISP << 17)) - (STARS_Z_DISP << 16)
    };
    
    return star;
}

void makeSintab() {
    // iterative sine generator from https://www.musicdsp.org/en/latest/Synthesis/10-fast-sine-and-cosine-calculation.html
    // c = (int)(65536.0*2.0*sin(PI/SINTAB_SIZE)), realculate it yourself for other SINTAB_SIZE
    int32_t a = 65536, b = 0, c = 402;
    for (size_t i = 0; i < SINTAB_SIZE; i++) {
        sintab[i] = b;
        a -= imul16(b,b);
        b += imul16(a,c);
    }
}

void makeStars() {
    for (size_t i = 0; i < STARS_COUNT; i++) {
        starsPos[i] = generateStar();
    }
}

// NB: I don't sort the stars so they tend to flicker if two stars overlap
void renderScreen(uint32_t tick, uint32_t delta) {   
    // clear framebuffer
    memset(screen, 0, sizeof(uint8_t) * X_RES * Y_RES);
    
    // calculate star rotation
    uint32_t tick_a = (tick >> 4);
    uint32_t tick_b = (tick >> 4);
    
    uint32_t sin_a = sintab[(tick_a) & (SINTAB_SIZE - 1)];
    uint32_t cos_a = sintab[(tick_a - COSTAB_OFFSET) & (SINTAB_SIZE - 1)];
    
    uint32_t sin_b = sintab[(tick_b) & (SINTAB_SIZE - 1)];
    uint32_t cos_b = sintab[(tick_b - COSTAB_OFFSET) & (SINTAB_SIZE - 1)];
    
    for (size_t i = 0; i < STARS_COUNT; i++) {
        
        // rotate over Z axis
        vec3x starOverZ = {
            imul16(starsPos[i].x, cos_a) - imul16(starsPos[i].y, sin_a),
            imul16(starsPos[i].x, sin_a) + imul16(starsPos[i].y, cos_a),
            starsPos[i].z
        };
        
        // rotate over Y axis
        vec3x star = {
            imul16(starOverZ.x, cos_b) - imul16(starOverZ.z, sin_b),
            starOverZ.y,
            imul16(starOverZ.x, sin_b) + imul16(starOverZ.z, cos_b),
        };
            
        // near plane clipping
        if (star.z >= STARS_Z_NEAR_PLANE) {  
            // calculate stars projection
            // xp = (x * projFov) / z; yp = (y * projFov) / z;        
            vec2i proj = {
                (imuldiv16(star.x, STARS_Z_PROJ_FOV, star.z)) + X_RES/2,
                (imuldiv16(star.y, STARS_Z_PROJ_FOV, star.z)) + Y_RES/2 
            };
            
            // distance attenuation
            uint8_t color = min(idiv16(STARS_Z_ATTENUATION, star.z), 255);
            
            // clip and plot
            if ((proj.x >= 0) && (proj.x < X_RES) && (proj.y >= 0) && (proj.y < Y_RES)) {
                screen[proj.x + (proj.y << 8) + (proj.y << 6)] = color;
            }
        }
        // advance stars
        starsPos[i].z -= STARS_Z_DELTA*delta; 
        
        // generate new star if out of displacement
        if (starsPos[i].z < (-STARS_Z_DISP << 16)) starsPos[i].z += 2*(STARS_Z_DISP << 16);
    }
}


void makePalette() {
    // init palette
    vgaPalColor pal[256];
    for (size_t i = 0; i < 256; i++) pal[i].val = (i >> 2) * 0x010101;
    
    // and set it
    vgaSetPalette(pal, 0, 256);
}

// and finally, the main loop

int main(int argc, char *argv[]) {
    size_t frameCounter = 0, tick = 0, oldTick = 0;
    
    if ((argc >= 2) && (strstr(argv[1], "?") != 0)) {
        printf("usage: stars.exe [NOVSYNC]\n");
        return 0;
    }
    bool  noVsync   = ((argc >= 2) && (strstr(strupr(argv[2]), "NOVSYNC") != 0));
    
    // init
    makeSintab();
    makeStars();
    rtc_initTimer(RTC_TIMER_RATE_1024HZ);
    vgaInit();
    vgaSetMode(0x13);
    makePalette();
    
    // main loop
    while (!kbhit()) {
        if (!noVsync) vgaVsync();
        vgaMemcpyA000(screen, 0, X_RES*Y_RES);
        
        // advance time
        oldTick = tick;
        tick = rtc_getTick();
        frameCounter++;
        
        // calculate time delta and render screen
        renderScreen(tick, tick - oldTick);
    }; getch();
    
    vgaSetMode(0x3);
    vgaDone();
    rtc_freeTimer();
    
    // print timing results
    {
        uint32_t num    = imuldiv16(frameCounter, 1024, tick);
        uint32_t denom  = imuldiv16(frameCounter, 1024 * 1000, tick) - (num * 1000);
        printf("%u RTC ticks in %u frames - %d.%d fps", tick, frameCounter, num, denom);
    }
}