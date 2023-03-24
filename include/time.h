#pragma once

#include <ch554.h>
#include <stdint.h>

extern __data volatile uint32_t _SYSTEM_TIME;

inline volatile uint32_t millis()
{
    return _SYSTEM_TIME;
}

inline void timer_init()
{
    // Use timer0 to record system time in milliseconds
    // 1. Enable interrupt globally
    EA = 1;
    // 2. Enable timer0 interrupt
    ET0 = 1;
    // 3. Set timer0 to Mode 1 - 16-bit counter
    //   - The reset value of TMOD = 0x00
    //   - Set bT0_M1 = 0 and bT0_M0 = 1
    TMOD |= bT0_M0;
    // 4. Start timer0
    TR0 = 1;
}

void timer0_interrupt(void) __interrupt(INT_NO_TMR0);
void delay(uint16_t ms);
void delayMicroseconds(uint16_t us);
