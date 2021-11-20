#ifndef __FXMATH_H
#define __FXMATH_H

#include <stdlib.h>
#include <math.h>

#ifndef min
#define min(a, b)      ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b)      ((a) > (b) ? (a) : (b))
#endif

#ifndef max
#define sgn(a)         ((a) < (0) ? (-1) : ((a) > (0) ? (1) : (0)))
#endif

#ifndef max
#define clamp(a, l, h) ((a) > (h) ? (h) : ((a) < (l) ? (l) : (a)))
#endif

const double PI = 3.141592653589793f;

// upside-down implementation of smoothstep()
inline float smoothstep(float edge0, float edge1, float x) {
  return (edge0 + (x * x * (3 - 2 * x)) * (edge1 - edge0));
}

/*
int32_t abs(int32_t a);
#pragma aux abs = \
    "mov    edx, eax" \
    "sar    edx, 31"  \
    "xor    eax, edx" \
    "sub    eax, edx" \
    parm [eax] value [eax] modify [eax edx]
*/
// fatmap2 ripoff :]

inline int32_t ceilx(int32_t a) {return (a + 0xFFFF) >> 16;}

// (x * y) >> 16
int32_t imul16(int32_t x, int32_t y);
#pragma aux imul16 = \
    " imul  edx        "\
    " shrd  eax,edx,16 "\
    parm [eax] [edx] value [eax]

// (x * y) >> 14
int32_t imul14(int32_t x, int32_t y);
#pragma aux imul14 = \
    " imul  edx        "\
    " shrd  eax,edx,14 "\
    parm [eax] [edx] value [eax]

// (x * y) / z
int32_t imuldiv16(int32_t x, int32_t y, int32_t z);
#pragma aux imuldiv16 = \
    " imul  edx        "\
    " idiv  ebx        "\
    parm [eax] [edx] [ebx] modify exact [eax edx] value [eax]
    
// (x << 16) / y
int32_t idiv16(int32_t x, int32_t y);
#pragma aux idiv16 = \
    " mov   edx,eax    "\
    " sar   edx,16     "\
    " shl   eax,16     "\
    " idiv  ebx        "\
    parm [eax] [ebx] modify exact [eax edx] value [eax]

    
// *dst = (int32_t) src;
void fist(int32_t * dst, double src);
#pragma aux fist = \
    "   fistp  dword ptr [eax]  "\
    parm [eax] [8087] modify [8087]
   
extern "C" {
    // float->int FADD trickery
    extern volatile int64_t _fadd_temp;
    extern const    float   _fadd_magic_32_0;
    extern const    float   _fadd_magic_16_16;
    extern const    float   _fadd_magic_24_8;
    extern const    float   _fadd_magic_8_24;
};

// src double -> dst 32.0 int
int32_t fistf(double src);
#pragma aux fistf = \
    " fadd  dword ptr [_fadd_magic_32_0] " \
    " fstp  qword ptr [_fadd_temp] " \
    " mov   eax, dword ptr [_fadd_temp] " \
    parm [8087] value [eax] modify [8087]

// src double -> dst 16.16fx    
int32_t fistfx(double src);
#pragma aux fistfx = \
    " fadd  dword ptr [_fadd_magic_16_16] " \
    " fstp  qword ptr [_fadd_temp] " \
    " mov   eax, dword ptr [_fadd_temp] " \
    parm [8087] value [eax] modify [8087]
    
// src double -> dst 24.8fx
int32_t fistfx24_8(double src);
#pragma aux fistfx8 = \
    " fadd  dword ptr [_fadd_magic_24_8] " \
    " fstp  qword ptr [_fadd_temp] " \
    " mov   eax, dword ptr [_fadd_temp] " \
    parm [8087] value [eax] modify [8087]

// src double -> dst 8.24fx
int32_t fistfx8_24(double src);
#pragma aux fistfxtex = \
    " fadd  dword ptr [_fadd_magic_8_24] " \
    " fstp  qword ptr [_fadd_temp] " \
    " mov   eax, dword ptr [_fadd_temp] " \
    parm [8087] value [eax] modify [8087]
   
#endif   
    