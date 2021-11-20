#ifndef RTCTIMER_H
#define RTCTIMER_H

/*

    RTC timer procedures v.1.01
    by wbc\\bz7 12.11.2o15 - 23.o3.2o16 - 15.ll.2oz1

    some guidelines :)
    - timer can run at rates from 2 to 8192 hz (look at rtc_timeRate and 
    rtc_clockDivisor defines)
    - your timer proc should avoid any calls to some stdlib functions and
    bios and dos interrupts beacuse it can lead to crash
    - works in real and protected mode under pure dos
    - works under windows (at least under mustdie'98 :) but 8192hz isn't stable

    conversion table:
    rtc_timerRate | rtc_clockDivisor
        1024      |        6
        2048      |        5
        4096      |        4
        8192      |        3d
    I guess nobody will use freqs below 1024hz ;), but if you want to use them
    look at this formula:

    rtc_timerRate = (32768 >> (rtc_clockDivisor - 1)); rtc_timerRate <= 8192 (!)

    known issues:
    - via vt82855n - when using 8192hz rate timer can stop after 10-15 seconds
    while 1024hz works fine
    
    changelog:
    v.1.03  - lots of fixes, made DJGPP compatible (i hope)
    v.1.02  - c++ compatible :)
    v.1.01  - added manual divisor selection in rtc_InitTimer()
    v.1.00  - initial release (used in blash\bz7)

*/

#ifdef __cplusplus
extern "C" {
#endif

enum {
    RTC_TIMER_RATE_8192HZ    = 3,
    RTC_TIMER_RATE_4096HZ,  /* 4  */
    RTC_TIMER_RATE_2048HZ,  /* 5  */
    RTC_TIMER_RATE_1024HZ,  /* 6  */
    RTC_TIMER_RATE_512HZ,   /* 7  */
    RTC_TIMER_RATE_256HZ,   /* 8  */
    RTC_TIMER_RATE_128HZ,   /* 9  */
    RTC_TIMER_RATE_64HZ,    /* 10 */
    RTC_TIMER_RATE_32HZ,    /* 11 */
    RTC_TIMER_RATE_16HZ,    /* 12 */
    RTC_TIMER_RATE_8HZ,     /* 13 */
    RTC_TIMER_RATE_4HZ,     /* 14 */
    RTC_TIMER_RATE_2HZ,     /* 15 */
};

// init RTC timer
void rtc_initTimer(int divisor);

// get current RTC tick count
unsigned long rtc_getTick();

// set timer function called every [divisor] RTC ticks
void rtc_setTimer(void (*func)(), int divisor);

// stop and free RTC timer
void rtc_freeTimer();

#ifdef __cplusplus
}
#endif

#endif

