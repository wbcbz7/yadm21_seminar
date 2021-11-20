#pragma once
/*
    example VGA stuff
    --wbcbz7 19.11.2021
*/

// per-compiler definitions
#if defined(__WATCOMC__)

#include <i86.h>
#include <string.h>
#include <conio.h>

#define _DPMI_DJGPP_COMPATIBILITY

#else if defined(__DJGPP__)

#include <conio.h>

#endif

// ----------------------------
// common definitions and includes

#include <stdint.h>
#include <dpmi.h>

// ----------------------------
// register definitions

enum {
    VGA_CRTC_INDEX          = 0x3D4,
    VGA_CRTC_DATA           = 0x3D5,
    
    VGA_SEQ_INDEX           = 0x3C4,
    VGA_SEQ_DATA            = 0x3C5,
    
    VGA_GC_INDEX            = 0x3CE,
    VGA_GC_DATA             = 0x3CF,
    
    VGA_INPUT_STATUS        = 0x3DA,
    VGA_MISC_OUTPUT         = 0x3C2,
    VGA_MISC_OUTPUT_READ    = 0x3CC,
    
    VGA_ATC_INDEX_DATA      = 0x3C0,
    VGA_ATC_DATA_READ       = 0x3C1,
    
    VGA_RAMDAC_MASK         = 0x3C6,
    VGA_RAMDAC_READ_INDEX   = 0x3C7,
    VGA_RAMDAC_WRITE_INDEX  = 0x3C8,
    VGA_RAMDAC_DATA         = 0x3C9,
    
    
    // register definitions
    VGA_CRTC_H_TOTAL   = 0,
    VGA_CRTC_H_DE_END,
    VGA_CRTC_H_BLANK_START,
    VGA_CRTC_H_BLANK_END,
    VGA_CRTC_H_SYNC_START,
    VGA_CRTC_H_SYNC_END,
    VGA_CRTC_V_TOTAL,
    VGA_CRTC_V_OVERFLOW,
    VGA_CRTC_PRESET_ROW_SCAN,
    VGA_CRTC_MAX_SCAN_LINE,
    VGA_CRTC_CURSOR_START,
    VGA_CRTC_CURSOR_END,
    VGA_CRTC_START_ADDRESS_HIGH,
    VGA_CRTC_START_ADDRESS_LOW,
    VGA_CRTC_CURSOR_POS_HIGH,
    VGA_CRTC_CURSOR_POS_LOW,
    VGA_CRTC_V_SYNC_START,
    VGA_CRTC_V_SYNC_END,
    VGA_CRTC_V_DE_END,
    VGA_CRTC_OFFSET,
    VGA_CRTC_UNDERLINE_LOCATION,
    VGA_CRTC_V_BLANK_START,
    VGA_CRTC_V_BLANK_END,
    VGA_CRTC_MODE_CONTROL,
    VGA_CRTC_LINE_COMPARE,
    
    VGA_CRTC_LATCH_AFF_READBACK     = 0x22,     // undocumented
    
    VGA_SEQ_RESET                   = 0,
    VGA_SEQ_CLOCK_MODE,
    VGA_SEQ_PLANE_WRITE_MASK,
    VGA_SEQ_CHARACTER_FONT,
    VGA_SEQ_MEMORY_MODE,
    
    VGA_GC_SET_RESET                = 0,
    VGA_GC_ENABLE_SET_RESET,
    VGA_GC_COLOR_COMPARE,
    VGA_GC_DATA_ROTATE,
    VGA_GC_READ_PLANE_SELECT,
    VGA_GC_GRAPHICS_MODE,
    VGA_GC_MISC,
    VGA_GC_COLOR_DONT_CARE,
    VGA_GC_BITMASK,
    
    VGA_ATC_PALETTE                 = 0,
    VGA_ATC_MODE_CONTROL            = 0x10,
    VGA_ATC_PLANE_COLOR_ENABLE,
    VGA_ATC_H_PIXEL_PANNING,
    VGA_ATC_COLOR_SELECT,
    
    VGA_ATC_PAL_SOURCE              = 0x20,
    
};


#ifdef __cplusplus
extern "C" {
#endif

typedef union {
    struct {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;      // or padding
    };
    uint32_t val;
} vgaPalColor;

// init (non-0 if failed)
uint32_t vgaInit();

// deinit (non-0 if failed)
uint32_t vgaDone();

// set requested video mode
inline void vgaSetMode(uint32_t mode);

// wait for vertical retrace start
inline void vgaVsync();

// get palette
void vgaGetPalette(vgaPalColor *pal, uint32_t start, uint32_t length);

// set palette
void vgaSetPalette(vgaPalColor *pal, uint32_t start, uint32_t length);

// copy buffer to screen
inline void vgaMemcpyA000(void *src, uint32_t offset, uint32_t size);
inline void vgaMemcpyB000(void *src, uint32_t offset, uint32_t size);
inline void vgaMemcpyB800(void *src, uint32_t offset, uint32_t size);

#if defined (__DJGPP__) 

// protected mode selectors for DJGPP
extern uint16_t vgaSelA000, vgaSelB000, vgaSelB8000;  

#endif

#ifdef __cplusplus
}
#endif

// -----------------------------------------------------------------------------------
// implementation (all these functions are really short, except for vgaInit()/vgaDone()
#if defined (__WATCOMC__)
void vgaSetMode(uint32_t mode);
#pragma aux vgaSetMode = "int 0x10" parm [eax]

// ...accessing VGA framebuffer in Watcom C is straightforward as ever
inline void vgaMemcpyA000(void *src, uint32_t offset, uint32_t size){
    memcpy((void*)(0xA0000 + offset), src, size);
}
inline void vgaMemcpyB000(void *src, uint32_t offset, uint32_t size){
    memcpy((void*)(0xB0000 + offset), src, size);
}
inline void vgaMemcpyB800(void *src, uint32_t offset, uint32_t size){
    memcpy((void*)(0xB8000 + offset), src, size);
}

#endif

#if defined (__DJGPP__) 
inline void vgaSetMode(uint32_t mode) {
    asm volatile (
        "int $0x10"
        :               // no output registers
        : "a"(mode)     // mode forwarded to EAX
        :               // no clobbered registers
    );
}

// ... while with DJGPP it involves segments, just like in 16-bit code
inline void vgaMemcpyA000(void *src, uint32_t offset, uint32_t size) {
    movedata(_my_ds(), src, vgaSelA000, offset, size);
}
inline void vgaMemcpyB000(void *src, uint32_t offset, uint32_t size) {
    movedata(_my_ds(), src, vgaSelB000, offset, size);
}
inline void vgaMemcpyB800(void *src, uint32_t offset, uint32_t size) {
    movedata(_my_ds(), src, vgaSelB800, offset, size);
}
#endif

// wait for vertical retrace
inline void vgaVsync() {
    // wait for end of current retrace
    while ((inp(0x3DA) & 8) == 8);
    // wait for start of next retrace
    while ((inp(0x3DA) & 8) != 8);
}
