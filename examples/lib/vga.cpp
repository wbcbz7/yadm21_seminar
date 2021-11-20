#include <vga.h>

#if defined (__DJGPP__) 

// protected mode selectors for DJGPP
extern uint16_t vgaSelA000, vgaSelB000, vgaSelB8000;  

// init selectors
uint32_t vgaInit() {
    vgaSelA000 = __dpmi_segment_to_descriptor(0xA000);
    vgaSelB000 = __dpmi_segment_to_descriptor(0xB000);
    vgaSelB800 = __dpmi_segment_to_descriptor(0xB800);
}
uint32_t vgaDone() { return 0; }

#else if defined (__WATCOMC__) 

// no init required
uint32_t vgaInit() { return 0; }
uint32_t vgaDone() { return 0; }

#endif


// palette get/set

void vgaGetPalette(vgaPalColor *pal, uint32_t start, uint32_t length) {
    outp(VGA_RAMDAC_READ_INDEX, start);
    for (uint32_t i = start; i < length; i++) {
        pal->r = inp(VGA_RAMDAC_DATA);
        pal->g = inp(VGA_RAMDAC_DATA);
        pal->b = inp(VGA_RAMDAC_DATA);
        pal++;
    }
}

void vgaSetPalette(vgaPalColor *pal, uint32_t start, uint32_t length) {
    outp(VGA_RAMDAC_WRITE_INDEX, start);
    for (uint32_t i = start; i < length; i++) {
        outp(VGA_RAMDAC_DATA, pal->r);
        outp(VGA_RAMDAC_DATA, pal->g);
        outp(VGA_RAMDAC_DATA, pal->b);
        pal++;
    }
}
