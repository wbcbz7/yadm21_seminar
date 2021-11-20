#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// RTC timer header
#include <rtctimer.h>

// cunstom timer tick counter
volatile uint32_t myTimerTick = 0;

// custom timer procedure - called inside timer ISR
void myTimer() {
    myTimerTick++;
}

int main(int argc, char* argv[]) {
    // install timer ISR
    printf("install timer...");
    rtc_initTimer(RTC_TIMER_RATE_1024HZ);
    printf("done\n");
    
    // call custom handler every 32 timer ticks
    printf("install custom handler...");
    rtc_setTimer(myTimer, 32);
    printf("done\n");
    
    while (!kbhit()) {
        printf("\rRTC timer ticks = %u, my timer ticks = %u", rtc_getTick(), myTimerTick);
        fflush(stdout);        
    }; getch();
    printf("\n");
    
    // remove timer ISR
    printf("remove timer...");
    rtc_freeTimer();
    printf("done\n");
    
    return 0;
};

