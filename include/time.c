#include "ch554.h"

#include <time.h>

// Time in milliseconds
//   2^32 / 1000 / 3600 / 24 = 49.71 days
__data volatile uint32_t _SYSTEM_TIME = 0;

void timer0_interrupt(void) __interrupt(INT_NO_TMR0)
{
    // In Mode 1, timer0/1 interrupt is triggered when TH0/1 & TL0/1 changes from 0xFFFF to 0x0000.
    // After reset (bT0_CLK = 0), the timer0's internal clock frequency is MCU_Frequency/12.
    // Calculate TH & TL,
    //   (0xFFFF + 1 - 0xTHTL) x (1 / (MCU_Frequency / 12))) = 0.001s
    //   -> 0xTHTL = 65536 - 0.001 x (MCU_Frequency / 12)
    // MCU Frequency    TH    TL     Decimal
    //        32 MHz  0xF5  0x95  62,869.333
    //        24 MHz  0xF8  0x30  63,536
    //        16 MHz  0xFA  0xCB  64,202.667
    //        12 MHz  0xFC  0x18  64,536
    //         6 MHz  0xFE  0x0C  65,036
    //         3 MHz  0xFF  0x06  65,286
    //       750 KHz  0xFF  0xC2  65,473.5
    //      187.5KHz  0xFF  0xF0  65,520.375

    // Reset TH0 & TL0 for the next interrupt
#if FREQ_SYS == 32000000
    TH0 = 0xF5;
    TL0 = 0x95;
#elif FREQ_SYS == 24000000
    TH0 = 0xF8;
    TL0 = 0x30;
#elif FREQ_SYS == 16000000
    TH0 = 0xFA;
    TL0 = 0xCB;
#elif FREQ_SYS == 12000000
    TH0 = 0xFC;
    TL0 = 0x18;
#elif FREQ_SYS == 6000000
    TH0 = 0xFE;
    TL0 = 0x0C;
#elif FREQ_SYS == 3000000
    TH0 = 0xFF;
    TL0 = 0x06;
#elif FREQ_SYS == 750000
    TH0 = 0xFF;
    TL0 = 0xC2;
#elif FREQ_SYS == 187500
    TH0 = 0xFF;
    TL0 = 0xF0;
#else
#warning FREQ_SYS invalid or not set
#endif

    _SYSTEM_TIME++;
}

void delayMicroseconds(uint16_t us)
{
#ifdef FREQ_SYS
#if FREQ_SYS <= 6000000
    us >>= 2;
#endif
#if FREQ_SYS <= 3000000
    us >>= 2;
#endif
#if FREQ_SYS <= 750000
    us >>= 4;
#endif
#endif
    while (us)
    {                // total = 12~13 Fsys cycles, 1uS @Fsys=12MHz
        ++SAFE_MOD;  // 2 Fsys cycles, for higher Fsys, add operation here
#ifdef FREQ_SYS
#if FREQ_SYS >= 14000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 16000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 18000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 20000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 22000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 24000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 26000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 28000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 30000000
        ++SAFE_MOD;
#endif
#if FREQ_SYS >= 32000000
        ++SAFE_MOD;
#endif
#endif
        --us;
    }
}

void delay(uint16_t ms)
{
    while (ms)
    {
#ifdef DELAY_MS_HW
        while ((TKEY_CTRL & bTKC_IF) == 0)
            ;
        while (TKEY_CTRL & bTKC_IF)
            ;
#else
        delayMicroseconds(1000);
#endif
        --ms;
    }
}
