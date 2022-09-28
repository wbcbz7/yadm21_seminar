/*
    cpuid
    --wbcbz7 l5.ll.zozl
*/

#include <stdint.h>
#include <string.h>

#include "mycpuid.h"

#if defined(__DJGPP__) 
// include GCC header
#include <cpuid.h>
#endif

uint32_t cpuid(uint32_t leaf,  uint32_t *_eax, uint32_t *_ebx, uint32_t *_ecx, uint32_t *_edx)
#if defined(__WATCOMC__)                     
{
    uint32_t a, b, c, d;
    
    _asm {
        mov     eax, [leaf]
        cpuid
        
        mov     [a], eax
        mov     [b], ebx
        mov     [c], ecx
        mov     [d], edx
    }
    
    *_eax = a; *_ebx = b; *_ecx = c; *_edx = d;
    
    return a;
}
#else if defined(__DJGPP__) 
{
    // use GCC built-in
    __get_cpuid(leaf, _eax, _ebx, _ecx, _edx);
    return _eax;
}
#endif


#if defined(__WATCOMC__)  
uint32_t eflagsTest(uint32_t mask);
#pragma aux eflagsTest = \
        "pushfd" \
        "pushfd" \
        "xor     dword ptr [esp],ebx" \
        "popfd" \
        "pushfd" \
        "pop     eax" \
        "xor     eax, dword ptr [esp]" \
        "popfd" \
        "and     eax, ebx" \
        parm [ebx] value [eax] modify [eax ebx]
        
#else if defined(__DJGPP__)
inline uint32_t eflagsTest(uint32_t mask) {
    uint32_t _eax;
    asm volatile(
        "pushfl" "\n\t"
        "pushfl" "\n\t"
        "xorl    %1, (%%esp)"  "\n\t"
        "popfl" "\n\t"
        "pushfl" "\n\t"
        "popl    %%0" "\n\t"
        "xorl    (%%esp), %%0" "\n\t"
        "popfl" "\n\t"
        "andl     %1, %%0" "\n\t"
        : "=a" (_eax)
        : "b" (mask)
        );
    return _eax;
}
#endif
inline uint32_t check486()    { return eflagsTest(0x00040000); };
inline uint32_t checkCpuid()  { return eflagsTest(0x00200000); };

// returns processor family
uint32_t cpuidget(cpuid_t *p) {
    uint32_t _eax, _ebx, _ecx, _edx;
    
    if (p == NULL) return 0;
    
    // clear p struct
    memset(p, 0, sizeof(cpuid_t));
    
    // check for 486
    // no need for 386 check, dos extender does it by default
    if (!check486()) p->family = 3; else p->family = 4;
    
    // check for cpuid availability
    if (!checkCpuid()) return p->family;
    
    // cpuid is supported
    p->supported = 1;
    
    // get id string
    cpuid(0, &_eax, &_ebx, &_ecx, &_edx);

    p->highestLeaf         = _eax;
    *(uint32_t*)&p->str[0] = _ebx;
    *(uint32_t*)&p->str[4] = _edx;
    *(uint32_t*)&p->str[8] = _ecx;
    
    // get additional info
    cpuid(1, &_eax, &_ebx, &_ecx, &_edx);
    
    p->extfamily    = (_eax >> 20) & 0xFF;
    p->extmodel     = (_eax >> 16) & 0xF;
    p->family       = (_eax >> 8) & 0xF;
    p->model        = (_eax >> 4) & 0xF;
    p->stepping     = (_eax >> 0) & 0xF;
    
    p->flags        = (_edx);
    p->extflags     = (_ecx);
    
    return (_eax >> 8) & 0xF;
}

