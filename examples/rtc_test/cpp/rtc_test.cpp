#include <conio.h>
#include <iostream>
#include <cstdint>

// RTC timer header
#include <rtctimer.h>

// cunstom timer tick counter
volatile std::uint32_t myTimerTick = 0;

// custom timer procedure - called inside timer ISR
void myTimer() {
    myTimerTick++;
}

int main(int argc, char* argv[]) {
    // install timer ISR
    std::cout << "install timer...";
    rtc_initTimer(RTC_TIMER_RATE_1024HZ);
    std::cout << "done" << std::endl;
    
    // call custom handler every 32 timer ticks
    std::cout << "install custom handler...";
    rtc_setTimer(myTimer, 32);
    std::cout << "done" << std::endl;
    
    while (!kbhit()) {
        std::cout << "\rRTC timer ticks = " << rtc_getTick() << ", my timer ticks = " << myTimerTick;
        std::cout.flush();        
    }; getch();
    std::cout << std::endl;
    
    // remove timer ISR
    std::cout << "remove timer...";
    rtc_freeTimer();
    std::cout << "done" << std::endl;
    
    return 0;
};