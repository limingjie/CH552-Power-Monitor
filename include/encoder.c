#include "encoder.h"

#include <gpio.h>

#define ENCODER_CLK P11
#define ENCODER_DT  P33
#define ENCODER_SW  P34
uint8_t _delta = 0;

void encoder_init()
{
    // Rotary encoder
    PIN_input_PU(ENCODER_CLK);
    PIN_input_PU(ENCODER_DT);
    PIN_input_PU(ENCODER_SW);
}

uint8_t encoder_get_delta()
{
    return _delta;
}

// https://forum.arduino.cc/t/reading-rotary-encoders-as-a-state-machine/937388
__bit encoder_process()
{
    __data static uint8_t state    = 0;
    __bit                 CLKstate = PIN_read(ENCODER_CLK);
    __bit                 DTstate  = PIN_read(ENCODER_DT);
    switch (state)
    {
        case 0:             // Idle state, encoder not turning
            if (!CLKstate)  // Turn clockwise and CLK goes low first
            {
                state = 1;
            }
            else if (!DTstate)  // Turn anticlockwise and DT goes low first
            {
                state = 4;
            }
            break;
        // Clockwise rotation
        case 1:
            if (!DTstate)  // Continue clockwise and DT will go low after CLK
            {
                state = 2;
            }
            break;
        case 2:
            if (CLKstate)  // Turn further and CLK will go high first
            {
                state = 3;
            }
            break;
        case 3:
            if (CLKstate && DTstate)  // Both CLK and DT now high as the encoder completes one step clockwise
            {
                state = 0;
                ++_delta;
                return 1;
            }
            break;
        // Anticlockwise rotation
        case 4:  // As for clockwise but with CLK and DT reversed
            if (!CLKstate)
            {
                state = 5;
            }
            break;
        case 5:
            if (DTstate)
            {
                state = 6;
            }
            break;
        case 6:
            if (CLKstate && DTstate)
            {
                state = 0;
                --_delta;
                return 1;
            }
            break;
    }

    return 0;
}
