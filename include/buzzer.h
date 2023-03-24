#pragma once

#include <ch554.h>
#include <stdint.h>

// Music Notes
// https://en.wikipedia.org/wiki/Piano_key_frequencies
#define C4 0   // Frequency = 261.6256 Hz, Period = 3822.2559 µs
#define D4 1   // Frequency = 293.6648 Hz, Period = 3405.2430 µs
#define E4 2   // Frequency = 329.6276 Hz, Period = 3033.7265 µs
#define F4 3   // Frequency = 349.2282 Hz, Period = 2863.4572 µs
#define G4 4   // Frequency = 391.9954 Hz, Period = 2551.0503 µs
#define A4 5   // Frequency = 440.0000 Hz, Period = 2272.7273 µs
#define B4 6   // Frequency = 493.8833 Hz, Period = 2024.7698 µs
#define C5 7   // Frequency = 523.2511 Hz, Period = 1911.1283 µs
#define D5 8   // Frequency = 587.3295 Hz, Period = 1702.6218 µs
#define E5 9   // Frequency = 659.2551 Hz, Period = 1516.8635 µs
#define F5 10  // Frequency = 698.4565 Hz, Period = 1431.7284 µs
#define G5 11  // Frequency = 783.9909 Hz, Period = 1275.5250 µs
#define A5 12  // Frequency = 880.0000 Hz, Period = 1136.3636 µs
#define B5 13  // Frequency = 987.7666 Hz, Period = 1012.3849 µs

void buzzer_init();
void buzzer_play(__xdata const uint8_t* __xdata melody);
