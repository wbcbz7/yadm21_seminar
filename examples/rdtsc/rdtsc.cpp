#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "mycpuid.h"

// CPUID info
cpuid_t cpuidInfo;

uint64_t rdtsc();
#pragma aux rdtsc = ".586" "rdtsc" value [edx eax]

// calibrate TSC using 18.2Hz IRQ0 timer
uint64_t calibrateTSC() {
    volatile uint32_t *biostimer = (uint32_t*)(0x46C);
    
    // wait until end of current tick cycle
    volatile uint32_t timerval = *biostimer;
    while (timerval == *biostimer);

    // count TSC cycles
    uint64_t tsc_start = rdtsc();
    timerval = *biostimer;
    while (timerval == *biostimer);

    // and return difference between two interrupts
    return ((rdtsc() - tsc_start) * 1000) / 55;
}

int main(int argc, char* argv[]) {
    // TSC cycles per second
    uint64_t tsc_cyclesPerSecond;

    // check CPUID flags
    cpuidget(&cpuidInfo);
    if ((cpuidInfo.flags & CPUID_FEATURE_TSC) == 0) {
        printf("error: TSC not present!\n");
        return 1;
    }

    // calibrate TSC counter
    printf("calibrating TSC...%llu cycles per second\n", (tsc_cyclesPerSecond = calibrateTSC()));

    // test TSC counter
    printf("testing TSC counter...\n");
    uint64_t tsc_start = rdtsc();
    while (!kbhit()) {
        uint64_t tsc_now = rdtsc() - tsc_start;
        double   seconds = (double)tsc_now / tsc_cyclesPerSecond;

        printf("\rseconds: %5.3lf, TSC timer ticks: %llu", seconds, tsc_now);
        fflush(stdout);        
    }; getch();
    printf("\n");

    printf("done\n");
    return 0;
}
