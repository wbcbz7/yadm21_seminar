#include <stdio.h>
#include <stdint.h>
#include <conio.h>

#include "vga.h"
#include "vbe.h"

/*
    quick and dirty VESA BIOS Extensions helper library (vbe.cpp) example :)
    -- wbcbz7 29.o9.2o22
*/

// test routines
int test8bpp(uint8_t *pixels, vbe_ModeInfoBlock *modeinfo) {
    uint8_t *p; uint32_t frame = 0;

    // set XOR pattern
    p = pixels;
    for (int y = 0; y < modeinfo->YResolution; y++) {
        for (int x = 0; x < modeinfo->XResolution; x++)
            *p++ = (x ^ y);
        p += modeinfo->BytesPerScanline - modeinfo->XResolution;
    }
    while (!kbhit()) {} getch();

    // animate XOR pattern
    frame = 0;
    while (!kbhit()) {
        vgaVsync();
        p = pixels;
        for (int y = 0; y < modeinfo->YResolution; y++) {
            for (int x = 0; x < modeinfo->XResolution; x++)
                *p++ = ((x ^ y) + frame) & 0xFF;
            p += modeinfo->BytesPerScanline - modeinfo->XResolution;
        }
        frame++;
    }
    getch();

    // set palette
    vgaPalColor palette[256];
    for (size_t i = 0; i < 256; i++) {
        palette[i].r = (i >> 2);
        palette[i].g = (i >> 3);
        palette[i].b = (i >> 4);
    }

    // using VBE functin 0x08 for setting the palette, and if it fails, revert back to VGA palette
    // if current mode defined as VGA compatible, then you can safely use VGA ports 3C7-3C9 for palette access
    vbe_SetPalette(palette, 0, 256, vbe_WAITRETRACE);
    if (vbe_GetStatus() != 0x4F) {exit(0); vgaVsync(); vgaSetPalette(palette, 0, 256);}
    
    while (!kbhit()) {} getch();

    // animate XOR pattern again :)
    frame = 0;
    while (!kbhit()) {
        vgaVsync();
        p = pixels;
        for (int y = 0; y < modeinfo->YResolution; y++) {
            for (int x = 0; x < modeinfo->XResolution; x++)
                *p++ = ((x ^ y) + frame) & 0xFF;
            p += modeinfo->BytesPerScanline - modeinfo->XResolution;
        }
        frame++;
    }
    getch();

    // if card supports wide palette - switch to 8 bit pal
    if ((vbe_SetDACWidth(8) == 8) && (vbe_GetStatus() == 0x4F)) {
        for (size_t i = 0; i < 256; i++) {
            palette[i].r = (i >> 1);
            palette[i].g = (i >> 0);
            palette[i].b = (i >> 2);
        }
        vbe_SetPalette(palette, 0, 256, vbe_WAITRETRACE);
        if (vbe_GetStatus() != 0x4F) {vgaVsync(); vgaSetPalette(palette, 0, 256);}
        while (!kbhit()) {} getch();
    }

    return 0;
}

int test16bpp(uint16_t *pixels, vbe_ModeInfoBlock *modeinfo) {
    uint16_t *p; uint32_t frame = 0;

    // set XOR pattern
    p = pixels;
    for (int y = 0; y < modeinfo->YResolution; y++) {
        for (int x = 0; x < modeinfo->XResolution; x++)
            *p++ = (((x + frame) & 0x1F) + (((y + frame) & 0x3F) << 5)) + (((x ^ y) & 0x1F) << 11);
        p += (modeinfo->BytesPerScanline >> 1) - modeinfo->XResolution;
    }
    while (!kbhit()) {} getch();

    // animate XOR pattern
    frame = 0;
    while (!kbhit()) {
        vgaVsync();
        p = pixels;
        for (int y = 0; y < modeinfo->YResolution; y++) {
            for (int x = 0; x < modeinfo->XResolution; x++)
                *p++ = (((x + frame) & 0x1F) + (((y + frame) & 0x3F) << 5)) + (((x ^ y) & 0x1F) << 11);
            p += (modeinfo->BytesPerScanline >> 1) - modeinfo->XResolution;
        }
        frame++;
    }
    getch();

    return 0;
}

int test32bpp(uint32_t *pixels, vbe_ModeInfoBlock *modeinfo) {
    uint32_t *p; uint32_t frame = 0;

    // set XOR pattern
    p = pixels;
    for (int y = 0; y < modeinfo->YResolution; y++) {
        for (int x = 0; x < modeinfo->XResolution; x++)
            *p++ = (((x + frame) & 0xFF) + (((y + frame) & 0xFF) << 8)) + (((x ^ y) & 0xFF) << 16);
        p += (modeinfo->BytesPerScanline >> 2) - modeinfo->XResolution;
    }
    while (!kbhit()) {} getch();

    // animate XOR pattern
    frame = 0;
    while (!kbhit()) {
        vgaVsync();
        p = pixels;
        for (int y = 0; y < modeinfo->YResolution; y++) {
            for (int x = 0; x < modeinfo->XResolution; x++)
                *p++ = (((x + frame) & 0xFF) + (((y + frame) & 0xFF) << 8)) + (((x ^ y) & 0xFF) << 16);
            p += (modeinfo->BytesPerScanline >> 2) - modeinfo->XResolution;
        }
        frame++;
    }
    getch();

    return 0;
}

int testVbeMode(uint32_t mode, vbe_ModeInfoBlock *modeinfo) {
    vbe_ModeInfo(mode, modeinfo);
    if (vbe_GetStatus() != 0x4f) {printf("can't get info for mode 0x%X!\n", mode); return 1;}
    vbe_SetMode(mode);
    if (vbe_GetStatus() != 0x4f) {printf("can't set mode 0x%X!\n", mode); return 1;}
    
    void *pixels = (uint8_t*)vbe_GetVideoPtr();
    if (pixels == NULL) {vbe_SetMode(0x3); printf("can't get pointer to framebuffer!\n"); return 1;}

    switch (modeinfo->BitsPerPixel) {
        case 8:     test8bpp ((uint8_t*) pixels, modeinfo); break;
        case 16:    test16bpp((uint16_t*)pixels, modeinfo); break;
        case 32:    test32bpp((uint32_t*)pixels, modeinfo); break;
        default:    break;
    }

    vbe_SetMode(0x3);
    printf("mapped screen pointer = %08X\n\n", pixels);
    printf("mode properties:\n");
    printf("---------------------------\n");
    printf(" Resolition:                    %d x %d pixels\n", modeinfo->XResolution, modeinfo->YResolution);
    printf(" Bits per pixel:                %d\n", modeinfo->BitsPerPixel);
    printf(" Bytes per scanline (pitch):    %d\n", modeinfo->BytesPerScanline);
    printf(" Mode attributes:               %X\n", modeinfo->ModeAttributes);
    printf(" Memory model:                  %X\n", modeinfo->MemoryModel);
    printf(" Video pages :                  %d\n", modeinfo->NumberOfImagePages + 1);
    printf(" Physical LFB pointer:          %08X\n", modeinfo->PhysBasePtr);

    // calculate bit masks
    uint32_t redMask   = ((1 << modeinfo->RedMaskSize)   - 1) << modeinfo->RedFieldPosition; 
    uint32_t greenMask = ((1 << modeinfo->GreenMaskSize) - 1) << modeinfo->GreenFieldPosition;
    uint32_t blueMask  = ((1 << modeinfo->BlueMaskSize)  - 1) << modeinfo->BlueFieldPosition;
    uint32_t rsvdMask  = ((1 << modeinfo->RsvdMaskSize)  - 1) << modeinfo->RsvdFieldPosition;

    if (modeinfo->MemoryModel == vbe_MM_DirectColor) {
        printf(" Red bit mask:                  %08X\n", redMask);
        printf(" Green bit mask:                %08X\n", greenMask);
        printf(" Blue bit mask:                 %08X\n", blueMask);
        printf(" Reserved bit mask:             %08X\n", rsvdMask);
    }
    printf("\n");
    printf("press any key...\n");
    while (!kbhit()) {} getch();
    printf("\n");

    return 0;
}

int main() {
    // info blocks
    vbe_VbeInfoBlock vbeinfo;
    vbe_ModeInfoBlock modeinfo;    
    int x, y, mode, banknum = 0, addr = 0;
    
    // frame buffer pointers
    uint8_t*  pixels8  = NULL;
    uint16_t* pixels16 = NULL;
    uint32_t* pixels32 = NULL;

    if (vbe_Init()) { printf("can't init vbe interface \n"); return 1; }
    vbe_ControllerInfo(&vbeinfo);
    
    printf(" VESA signature:               %4s\n",    vbeinfo.vbeSignature);
    printf(" VESA BIOS Extensions version: %d.%d\n",  (vbeinfo.vbeVersion >> 8), (vbeinfo.vbeVersion & 0xFF));
    printf(" VESA mode table pointer:      0x%X\n",   (unsigned long)(vbeinfo.VideoModePtr));
    printf(" VESA OEM string:              %s\n",     vbeinfo.OemStringPtr);
    printf(" Total memory avaiable:        %d KiB\n", (vbeinfo.TotalMemory << 6));
    if ((vbeinfo.vbeVersion >> 8) >= 2) {
    printf(" OEM software revision:        %d.%d\n",  (vbeinfo.OemSoftwareRev >> 8), (vbeinfo.OemSoftwareRev & 0xFF));
    printf(" OEM vendor  name:             %s\n",     vbeinfo.OemVendorNamePtr);
    printf(" OEM product name:             %s\n",     vbeinfo.OemProductNamePtr);
    printf(" OEM revision string:          %s\n",     vbeinfo.OemProductRevPtr);
    }
    
    printf("\n");
    printf("setting mode 0x101 (640x480x8bpp) banked, press a key...\n");
    while (!kbhit()) {} getch();
    
    vbe_ModeInfo(0x101, &modeinfo);
    if (vbe_GetStatus() != 0x4f) {printf("can't get info for mode 0x101\n"); return 1;}
    vbe_SetMode(0x101);
    if (vbe_GetStatus() != 0x4f) {printf("can't set mode 0x101\n"); return 1;}
    
    pixels8 = (uint8_t*)vbe_GetVideoPtr();
    if (pixels8 == NULL) {vbe_SetMode(0x3); printf("can't get pointer to framebuffer!\n"); return 1;}
    
    // draw simple XOR pattern
    for (int y = 0; y < modeinfo.YResolution; y++){
        for (int x = 0; x < modeinfo.XResolution; x++) {
            pixels8[addr++] = x ^ y;
            if (addr >= (modeinfo.Granularity << 10)) {
                banknum++; addr = 0;
                vbe_SetWindowPos(0, banknum);
            }
        }
        // fixup pitch
        addr += modeinfo.BytesPerScanline - modeinfo.XResolution;
    }
    
    while (!kbhit()) {} getch();
    
    vbe_SetMode(0x3);
    printf("screen pointer = 0x%X\n", pixels8);
    printf("mode properties:\n");
    
    printf("setting mode 0x101 (640x480x8bpp) with LFB, press a key...\n");
    while (!kbhit()) {} getch();
    testVbeMode(0x101 | vbe_MODE_LINEAR, &modeinfo);
    
    printf("trying to find 1024x768x8bpp mode with LFB, press a key...\n");
    while (!kbhit()) {} getch();
    mode = vbe_FindMode(1024, 768, 8, vbe_ATTR_LFB_Support);
    if (mode == -1) {printf("can't find 1024x768x8bpp LFB mode\n"); return 1;}    
    testVbeMode(mode | vbe_MODE_LINEAR, &modeinfo);

    printf("checking for 15\\16 bit modes...\n");
    mode = vbe_FindMode(640, 480, 15, 0);
    printf("640x480x15bpp mode - "); if (mode != -1) printf("0x%x\n", mode); else printf("does not exist or fake\n");
    mode = vbe_FindMode(640, 480, 16, 0);
    printf("640x480x16bpp mode - "); if (mode != -1) printf("0x%x\n", mode); else printf("does not exist or fake\n");
    while (!kbhit()) {} getch();

    if (mode != -1) {
        printf("set mode 0x%X LFB, press any key...\n", mode);
        while (!kbhit()) {} getch();
        testVbeMode(mode | vbe_MODE_LINEAR, &modeinfo);
    }

    printf("checking for 32 bit modes...\n");
    mode = vbe_FindMode(640, 480, 32, 0);
    printf("640x480x32bpp mode - "); if (mode != -1) printf("0x%x\n", mode); else printf("does not exist\n");
    while (!kbhit()) {} getch();
    if (mode != -1) {
        printf("set mode 0x%X LFB, press any key...\n", mode);
        while (!kbhit()) {} getch();
        testVbeMode(mode | vbe_MODE_LINEAR, &modeinfo);
    }

    vbe_Done();
    printf("done\n");
    return 0;
}
