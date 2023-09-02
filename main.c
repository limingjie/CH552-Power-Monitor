#include <ch554.h>
#include <system.h>  // mcu_config()
#include <time.h>    // millis(), delay()
#include <oled.h>    // OLED
#include <buzzer.h>
#include <encoder.h>

#include "meter.h"

__xdata const uint8_t start_sound[] = {1, C4, 1};

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
    // __xdata char buf[25];

    startup();
    buzzer_play(start_sound);
    meter_display();

    while (1)
    {
        // if (encoder_process())
        // {
        //     // TODO: functions
        //     sprintf(buf, "enc = %3u", encoder_get_delta());
        //     OLED_printxy(0, 7, buf);
        // }

        if (millis() - last_system_time >= 50)
        {
            last_system_time = millis();
            meter_run();
        }
    }
}
