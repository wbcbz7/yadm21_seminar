#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//  8bpp rgb images stored in uint8_t      as RGB332
// 12bpp rgb images stored in uint16_t     as ARGB4444
// 15bpp rgb images stored in uint16_t     as ARGB1555
// 16bpp rgb images stored as uint16_t     as RGB565
// 24bpp rgb images stored as 3*uint8_t(!) as RGB888
// 32bpp rgb images stored as unsigned int       as ARGB8888

// for 15bpp alpha defined as 0 - transparent, 1   - opaque
// for 32bpp alpha defined as 0 - transparent, 255 - opaque,
// any value in (0, 255) - transparency;
// same for 12bpp but alpha is limited to [0; 16)

#ifdef __cplusplus
extern "C" {
#endif

// tga header
#pragma pack (push, 1)
typedef struct {
    uint8_t  IDLength;        /* 00h  Size of Image ID field */
    uint8_t  ColorMapType;    /* 01h  Color map type */
    uint8_t  ImageType;       /* 02h  Image type code */
    uint16_t CMapStart;       /* 03h  Color map origin */
    uint16_t CMapLength;      /* 05h  Color map length */
    uint8_t  CMapDepth;       /* 07h  Depth of color map entries */
    uint16_t XOffset;         /* 08h  X origin of image */
    uint16_t YOffset;         /* 0Ah  Y origin of image */
    uint16_t Width;           /* 0Ch  Width of image */
    uint16_t Height;          /* 0Eh  Height of image */
    uint8_t  PixelDepth;      /* 10h  Image pixel size */
    uint8_t  ImageDescriptor; /* 11h  Image descriptor byte */
} tga_header;
#pragma pack (pop)

enum {
    tgaload_ok, tgaload_nofile, tgaload_ioerr, tgaload_fmterr, tgaload_memerr
};

int tga_load8_header(const char *fname, tga_header *head);
int tga_load8_data  (const char *fname, tga_header *head, uint8_t *buf, uint32_t *pal, int order, size_t pitch = 0);

int tga_load_header(const char *fname, tga_header *head);
int tga_load_data  (const char *fname, tga_header *head, uint32_t *buf, int order, size_t pitch = 0);

int tga_load   (const char *fname, tga_header *head, uint32_t  **buf, int order);
int tga_load8  (const char *fname, tga_header *head, uint8_t  **buf, uint32_t **pal, int order);

#ifdef __cplusplus
}
#endif