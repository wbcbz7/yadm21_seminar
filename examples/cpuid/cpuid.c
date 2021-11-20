#include <stdio.h>
#include <stdint.h>

#include "mycpuid.h"

struct cpuidRequest_t {
    uint32_t eax, ebx, ecx, edx;
} cpuidRequest;

cpuid_t cpuidInfo;

int main(int argc, char *argv[]) {
    // retireve cpuid info
    cpuidget(&cpuidInfo);
    printf("CPU family: %d, CPUID %s supported\n", cpuidInfo.family, cpuidInfo.supported ? "is" : "not");
    
    if (cpuidInfo.supported) {
        printf("CPU model: %u, stepping %u\n", cpuidInfo.model, cpuidInfo.stepping); 
        printf("CPU flags: 0x%08X, extended flags: 0x%08X\n", cpuidInfo.flags, cpuidInfo.extflags); 
        printf("Highest supported leaf is %u\n", cpuidInfo.highestLeaf); 
        
        // test for some flags, i.e. MMX support
        printf("\n");
        printf("test CPU flags for MMX support: %s present\n", (cpuidInfo.flags & CPUID_FEATURE_MMX) ? "is" : "not"); 
    }
    
    return 0;
}