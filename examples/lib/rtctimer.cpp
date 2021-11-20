/*

    RTC timer procedures v.1.01
    by wbc\\bz7 12.11.2o15 - 23.o3.2o16 - 15.ll.2oz1

    some guidelines :)
    - timer can run at freqs from 2 to 8192 hz (look at rtc_timeRate and 
    rtc_clockDivisor defines)
    - your timer proc should avoid any calls to some stdlib functions and
    bios and dos interrupts bcoz it can lead to crash
    - works in real and protected mode under pure dos
    - works under windows (at least under mustdie'98 :) but 8192hz isn't stable

    conversion table:
    rtc_timerRate | rtc_clockDivisor
        1024      |        6
        2048      |        5
        4096      |        4
        8192      |        3
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

#include <dos.h>
#include <stdlib.h>
#include <conio.h> 

 // for inp()/outp()
#if defined(__WATCOMC__) 
#include <i86.h>
#else if defined(__DJGPP__) 
#include <pc.h>
#endif

// for __dpmi_lock_linear_region()/__dpmi_unlock_linear_region()
#if defined(__WATCOMC__) 
#define _DPMI_DJGPP_COMPATIBILITY
#endif
#include <dpmi.h>

#include "rtctimer.h"

static unsigned long rtc_timerRate    = 1024;  // default
static unsigned long rtc_clockDivisor = 6;

// --------------------------
// LOCKED DATA BEGIN

static unsigned char rtc_lockedData_begin;

static void (*rtc_timerProc)() = NULL; // timer procedure

#if defined(__WATCOMC__) 
static void (__interrupt __far *rtc_oldhandler)(); // internal procedure - old INT70h handler
#endif

#if defined(__DJGPP__) 
static _go32_dpmi_seginfo rtc_oldhandler;           // internal procedure - old INT70h handler
static _go32_dpmi_seginfo rtc_callback;             // GO32 managed callback 
#endif

// procedure downcounter and tick counter
volatile unsigned long rtc_downcount, rtc_tick;
static int rtc_divisor;

// ---------------------------
// STACK STUFF - WATCOM ONLY

#if defined(__WATCOMC__) 
enum {rtc_stackSize = 65536};

// stack stuff
static char *stackbuf;
static char __far *stack;
static unsigned long stacksize;
static unsigned char stackused;
static void __far *oldssesp;

void stackcall(void (*proc)());
#pragma aux stackcall parm [eax] = \
  "mov word ptr oldssesp+4,ss" \
  "mov dword ptr oldssesp+0,esp" \
  "lss esp,stack" \
  "sti" \
  "call eax" \
  "cli" \
  "lss esp,oldssesp"

void loades();
#pragma aux loades = "push ds" "pop es"

#else if defined(__DJGPP__) 
void stackcall(void (*proc)()) { proc(); }
void loades() {}
#endif

static unsigned char rtc_lockedData_end;

// -----------------------------

#if defined(__WATCOMC__) 
void __interrupt __far rtc_handler()
#else if defined(__DJGPP__) 
void rtc_handler()
#endif
{
    loades();
    _disable();
    rtc_tick++;
    rtc_downcount--;
    if (rtc_downcount == 0) { 
        if (rtc_timerProc != NULL) {
            if (!stackused) {
                stackused++;
                stackcall(rtc_timerProc);
                stackused--;
            }
        }
        rtc_downcount = rtc_divisor;
    }
    do outp(0x70, 0xC); while (inp(0x71) & 0x80); // clear interrupt flags in RTC
    outp(0xA0, 0x20); outp(0x20, 0x20);           // and send EOI to interrupt controller
    _enable();
}
void rtc_handler_end() {}


#if defined(__WATCOMC__) 
void rtc_initTimer_impl() {
    // stack init
    stackbuf = (char*)malloc(rtc_stackSize);
    if (!stackbuf) return;
    stack = (char _far *)((size_t)(stackbuf + rtc_stackSize) & ~32);    // align by 32 bytes
    
    // lock stack (no need to use universal function here)
    dpmi_lockmemory((void*)stackbuf, rtc_stackSize);
    
    // install ISR and start timer
    rtc_oldhandler = _dos_getvect(0x70);
    _dos_setvect(0x70, rtc_handler);
}

void rtc_freeTimer_impl() {
    // restore ISR
    _dos_setvect(0x70, rtc_oldhandler);
    
    // unlock stack
    dpmi_unlockmemory((void*)stackbuf, rtc_stackSize);
    
    // and remove it
    free(stackbuf);
}

void rtc_lockCodeData() {
    dpmi_lockmemory((void*)FP_OFF(&rtc_handler), ((size_t)&rtc_handler_end - (size_t)&rtc_handler));
    dpmi_lockmemory(&rtc_lockedData_begin, ((size_t)&rtc_lockedData_end - (size_t)&rtc_lockedData_begin));
}

void rtc_unlockCodeData() {
    dpmi_unlockmemory((void*)FP_OFF(&rtc_handler), ((size_t)&rtc_handler_end - (size_t)&rtc_handler));
    dpmi_unlockmemory(&rtc_lockedData_begin, ((size_t)&rtc_lockedData_end - (size_t)&rtc_lockedData_begin));
}

#endif

#if defined(__DJGPP__) 
void rtc_initTimer_impl() {
    // install ISR and start timer
    _go32_dpmi_get_protected_mode_interrupt_handler(0x70, &rtc_oldhandler);
    rtc_callback.pm_offset = (int)rtc_handler;
    _go32_dpmi_allocate_iret_wrapper(&rtc_callback);
    _go32_dpmi_set_protected_mode_interrupt_handler(0x70, &rtc_callback);
}

void rtc_freeTimer_impl() {
    _go32_dpmi_set_protected_mode_interrupt_handler(0x70, &rtc_oldhandler);
    _go32_dpmi_free_iret_wrapper(&rtc_callback);
}

void rtc_lockCodeData() {
    // lock code/data
    __dpmi_meminfo lockinfo;
    lockinfo.address = (size_t)&rtc_handler; lockinfo.size = (size_t)(&rtc_handler_end - &rtc_handler); 
    __dpmi_lock_linear_region(&lockinfo);
    lockinfo.address = (size_t)&rtc_lockedData_begin; lockinfo.size = (size_t)(&rtc_lockedData_end - &rtc_lockedData_begin); 
    __dpmi_lock_linear_region(&lockinfo);
}

void rtc_unlockCodeData() {
    __dpmi_meminfo lockinfo;
    lockinfo.address = (size_t)&rtc_handler; lockinfo.size = (size_t)(&rtc_handler_end - &rtc_handler); 
    __dpmi_unlock_linear_region(&lockinfo);
    lockinfo.address = (size_t)&rtc_lockedData_begin; lockinfo.size = (size_t)(&rtc_lockedData_end - &rtc_lockedData_begin); 
    __dpmi_unlock_linear_region(&lockinfo);
}

#endif

// --------------------------------------------
// common functions

// call it before any rtc_setTimer() calls
void rtc_initTimer(int divisor) {
    rtc_clockDivisor = divisor & 0xF; 
    rtc_timerRate = (32768 >> (rtc_clockDivisor - 1));
    rtc_timerProc = NULL;
    
    rtc_lockCodeData();
    
    // disable ints
    _disable();
    
    // implementation-defined stuff
    rtc_initTimer_impl();
    
    // program RTC
    outp(0xA1, (inp(0xA1) & 0xFE));                                        // unmask IRQ8
    outp(0x70, 0x8A); outp(0x71, ((inp(0x71) & 0xF0) | rtc_clockDivisor)); // disable NMI and select rate
    outp(0x70, 0x8B); outp(0x71,  (inp(0x71) | 0x40));                     // enable periodic interrupt
    outp(0x70, 0xD);  inp(0x71);                                           // enable NMI
    rtc_divisor = 0x7FFFFFFF; rtc_downcount = rtc_divisor; rtc_tick = 0;
    _enable();
}

// call it before exit or you will get...guess that? :)
void rtc_freeTimer() {
    _disable();
    
    // implementation-defined stuff
    rtc_freeTimer_impl();
    
    // we will not mask IRQ8 because we will return control to BIOS interrupt handler
    outp(0x70, 0x8A); outp(0x71, ((inp(0x71) & 0xF0) | 0x6)); // disable NMI and select 1024 hz rate
    outp(0x70, 0x8B); outp(0x71,  (inp(0x71) & 0xBF));        // disable periodic interrupt
    outp(0x70, 0xD);  inp(0x71);                              // enable NMI
    _enable();
    
    // unlock code/data
    rtc_unlockCodeData();
}

// replaces current timer
void rtc_setTimer(void (*func)(), int divisor) {
    _disable();
    rtc_timerProc = func;
    rtc_divisor = divisor; rtc_downcount = divisor;
    _enable();
}

unsigned long rtc_getTick() {
    return rtc_tick;
}
