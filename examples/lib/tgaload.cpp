#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "tgaload.h"

// --------------- 32bpp ----------------

int tga_load_header(const char *fname, tga_header *head) {
    FILE *f;
    
    // open file
    f = fopen(fname, "rb");
    if (!f) return tgaload_nofile;
    
    // read header
    fread(head, sizeof(tga_header), 1, f);
    if (ferror(f)) {fclose(f); return tgaload_ioerr;}
    
    // check format
    if ((head->ImageType != 2) || (head->PixelDepth < 24)) {fclose(f); return tgaload_fmterr;}
    
    fclose(f);
    
    return tgaload_ok;
};

// load input TGA file
int tga_load_data(const char *fname, tga_header *head, uint32_t *buf, int order, size_t pitch) {
    FILE *f;
    
    // open file
    f = fopen(fname, "rb");
    if (!f) return tgaload_nofile;
    
    // skip header
    fseek(f, 18, SEEK_SET);
    
    // set pitch
    if (pitch == 0) pitch = head->Width << 2;
    
    int raworder = (order ^ (head->ImageDescriptor & 0x20 ? -1 : 0)) & 1;
    fseek(f, sizeof(tga_header) + head->IDLength + (head->CMapLength * ((head->CMapDepth + 7) >> 3)), SEEK_SET);
    if (feof(f)) {fclose(f); return tgaload_ioerr;}
    
    uint32_t *p = (raworder ? buf + (pitch >> 2) * (head->Height-1) : buf);
    for (int i = 0; i < head->Height; i++) {
        if (head->PixelDepth == 24) {
            for (int j = 0; j < head->Width; j++) {
                int tmp = fgetc(f);
                tmp |= (fgetc(f) << 8);
                tmp |= (fgetc(f) << 16) | 0xFF000000;
                *p++ = tmp;
            }
            p -= (pitch >> 2);
        } else fread(p, head->Width, sizeof(uint32_t), f);
        if (ferror(f)) {fclose(f); return tgaload_ioerr;}
        if (raworder) p -= (pitch >> 2); else p += (pitch >> 2);
    }
    
    fclose(f);
    return tgaload_ok;
}

// same but 32bpp
int tga_load(const char *fname, tga_header *head, uint32_t **buf, int order) {
    
    tga_load_header(fname, head);
    
    // create image
    *buf = new uint32_t[head->Width*head->Height];
    
    return tga_load_data(fname, head, *buf, order);
}

// load input TGA file
int tga_load16(const char *fname, tga_header *head, uint16_t **buf, int order) {
    FILE *f;
    
    // open file
    f = fopen(fname, "rb");
    if (!f) return tgaload_nofile;
    
    // read header
    fread(head, sizeof(tga_header), 1, f);
    if (ferror(f)) {fclose(f); return tgaload_ioerr;}
    
    // check format
    if ((head->ImageType != 2) || (head->PixelDepth < 15)) {fclose(f); return tgaload_fmterr;}
    
    int raworder = (order ^ (head->ImageDescriptor & 0x20 ? -1 : 0)) & 1;
    fseek(f, sizeof(tga_header) + head->IDLength + (head->CMapLength * ((head->CMapDepth + 7) >> 3)), SEEK_SET);
    if (feof(f)) {fclose(f); return tgaload_ioerr;}
    
    *buf = new uint16_t[head->Width*head->Height];
    
    uint16_t *p = (raworder ? *buf + head->Width * (head->Height-1) : *buf);
    for (int i = 0; i < head->Height; i++) {
        fread(p, head->Width, sizeof(uint16_t), f);
        if (ferror(f)) {fclose(f); return tgaload_ioerr;}
        if (raworder) p -= head->Width; else p += head->Width;
    }
    
    fclose(f);
    return tgaload_ok;
}

// ------------------- 8 bpp --------------------------------

// load header only
int tga_load8_header(const char *fname, tga_header *head) {
    FILE *f;
    
    // open file
    f = fopen(fname, "rb");
    if (!f) return tgaload_nofile;
    
    // read header
    fread(head, sizeof(tga_header), 1, f);
    if (ferror(f)) {fclose(f); return tgaload_ioerr;}
    
    // check format
    if (head->PixelDepth != 8) {fclose(f); return tgaload_fmterr;}
    
    fclose(f);
    
    return tgaload_ok;
};

// load image to specified pointers
int tga_load8_data(const char *fname, tga_header *head, uint8_t *buf, uint32_t *pal, int order, size_t pitch) {
    FILE *f;
    
    // open file
    f = fopen(fname, "rb");
    if (!f) return tgaload_nofile;
    
    // skip header
    fseek(f, 18, SEEK_SET);
    
    int raworder = order ^ (head->ImageDescriptor & 0x20 ? -1 : 0);
    // set pitch
    if (pitch == 0) pitch = head->Width;
    
    uint32_t *v = pal;
    
    if (v != NULL) {
        // load palette
        if (head->ImageType == 3)
            // grayscale palette
            for (size_t i = 0; i < 256; i++) v[i] = i * 0x010101;
        else {
            // color palette
            fseek(f, sizeof(tga_header) + head->IDLength, SEEK_SET);
            if (head->CMapDepth == 24) for (int i = head->CMapStart; i < (head->CMapStart+head->CMapLength); i++) {
                uint32_t tmp = fgetc(f); tmp |= (fgetc(f) << 8); tmp |= ((fgetc(f) << 16) | 0xFF000000); v[i] = tmp;
            }
            else fread(&v[head->CMapStart], head->CMapLength, sizeof(uint32_t), f);
        }
    }
    
    fseek(f, sizeof(tga_header) + head->IDLength + (head->CMapLength * ((head->CMapDepth + 7) >> 3)), SEEK_SET);
    if (feof(f)) {fclose(f); return tgaload_ioerr;}
    
    uint8_t *p = (raworder ? buf + pitch * (head->Height-1) : buf);
    for (int i = 0; i < head->Height; i++) {
        fread(p, head->Width, sizeof(uint8_t), f);
        if (ferror(f)) {fclose(f); return tgaload_ioerr;}
        if (raworder) p -= pitch; else p += pitch;
    }
    
    fclose(f);
    return tgaload_ok;
}

// load input TGA file
int tga_load8(const char *fname, tga_header *head, uint8_t **buf, uint32_t **pal, int order) {
    
    tga_load8_header(fname, head);
    
    // create palette
    *pal = new uint32_t[256]; memset(*pal, 0, sizeof(uint32_t)*256);
    
    // create image
    *buf = new uint8_t[head->Width*head->Height];
    
    return tga_load8_data(fname, head, *buf, *pal, order);
}