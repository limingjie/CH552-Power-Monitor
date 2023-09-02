#include <buzzer.h>
#include <ch554.h>
#include <encoder.h>
#include <gpio.h>    // PIN_read(), PIN_input_PU()
#include <oled.h>    // OLED
#include <system.h>  // mcu_config()
#include <time.h>    // millis(), delay()

#include "meter.h"

#define RESET_PIN P34

// __xdata const uint8_t start_sound[] = {1, C4, 1};

__data uint32_t last_system_time = 0;

void startup()
{
    // Set MCU Frequency
    mcu_config();
    delay(5);

    OLED_init();
    OLED_clear();
    meter_init();
    timer_init();
    encoder_init();
    buzzer_init();
}

void main()
{
    PIN_input_PU(RESET_PIN);

    startup();
    // buzzer_play(start_sound);
    meter_display();

    while (1)
    {
        if (!PIN_read(RESET_PIN))  // Reset button pressed
        {
            delay(100);  // Naive debounce, not necessary as it can be reset multiple times.
            meter_reset();
        }

        if (millis() - last_system_time >= 20)  // Refresh every 20ms.
        {
            last_system_time = millis();
            meter_run();
        }
    }
}
