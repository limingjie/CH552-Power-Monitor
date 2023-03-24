#include "buzzer.h"

#include <time.h>
#include <gpio.h>

// Schematic
//                             VCC
//                              |
//                        ______|
//           1n4001      _|_    |
//       Flyback Diode   /_\  [Passive Buzzer]
//                        |_____|
//                              |
//                180Ω       b  | c
//  GPIO <-------[/\/\]-------|<     SS8050
//                              | e   NPN
//                              |
//                              |
//                             GND

#define BUZZER_PIN P15           // P1.5 - Buzzer
SBIT(BUZZER, 0x90, BUZZER_PIN);  // P1.5

// The length of half period of notes in µs.
// -> 1,000,000µs / Note_Frequency / 2
__code const uint16_t NOTES_HALF_PERIOD_us[] = {
    1911, 1703, 1517, 1432, 1276, 1136, 1012,  // C4 - B4
    956,  851,  758,  716,  638,  568,  506    // C5 - B5
};

// Number of waves every 100 ms.
// -> Note_Frequency / 10
__code const uint8_t NOTES_WAVES_IN_100ms[] = {
    26, 29, 33, 35, 39, 44, 49,  // C4 - B4
    52, 59, 66, 70, 78, 88, 99   // C5 - B5
};

void buzzer_init()
{
    PIN_output(BUZZER_PIN);
    BUZZER = 0;
}

// Play a melody
// - melody = [melody length, note_0, note_0_beats, note_1, note_1_beats...]
//   - a beat last 100ms.
//   - a 50ms pause is placed between 2 notes.
void buzzer_play(__xdata const uint8_t* __xdata melody)
{
    const uint8_t length = *melody++;
    for (uint8_t note = 0; note < length; note++)
    {
        uint8_t  waves    = NOTES_WAVES_IN_100ms[*melody];    // The number of waves in 100ms.
        uint16_t delay_us = NOTES_HALF_PERIOD_us[*melody++];  // The half period of the wave.
        // The 2 while loops play the note waves x beats times, which last 100ms x beats.
        while (waves--)
        {
            uint8_t beats = *melody;
            while (beats--)
            {
                BUZZER = 1;
                delayMicroseconds(delay_us);
                BUZZER = 0;
                delayMicroseconds(delay_us);
            }
        }

        melody++;

        // Pause between 2 notes
        delay(50);
    }
}
