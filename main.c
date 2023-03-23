#include <stdio.h>  // sprintf

#include <ch554.h>
#include <system.h>  // mcu_config()
#include <time.h>    // millis(), delay()
#include <oled.h>    // OLED
#include <ina219.h>  // INA219
#include <gpio.h>
#include <buzzer.h>

#define SHUNT0_EN P30
#define SHUNT1_EN P31
#define SHUNT2_EN P32

#define ENCODER_CLK P11
#define ENCODER_DT  P33
#define ENCODER_SW  P34
uint8_t encoder_value = 0;

__xdata const uint8_t warningSound[] = {1, C4, 1};

// https://forum.arduino.cc/t/reading-rotary-encoders-as-a-state-machine/937388
uint8_t process_encoder()
{
    __data static uint8_t state    = 0;
    __bit                 CLKstate = PIN_read(ENCODER_CLK);
    __bit                 DTstate  = PIN_read(ENCODER_DT);
    switch (state)
    {
        case 0:  // Idle state, encoder not turning
            if (!CLKstate)
            {  // Turn clockwise and CLK goes low first
                state = 1;
            }
            else if (!DTstate)
            {  // Turn anticlockwise and DT goes low first
                state = 4;
            }
            break;
        // Clockwise rotation
        case 1:
            if (!DTstate)
            {  // Continue clockwise and DT will go low after CLK
                state = 2;
            }
            break;
        case 2:
            if (CLKstate)
            {  // Turn further and CLK will go high first
                state = 3;
            }
            break;
        case 3:
            if (CLKstate && DTstate)
            {  // Both CLK and DT now high as the encoder completes one step clockwise
                state = 0;
                ++encoder_value;
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
                --encoder_value;
                return 1;
            }
            break;
    }

    return 0;
}

void startup()
{
    // Set MCU Frequency
    mcu_config();
    delay(5);

    // Rotary encoder
    PIN_input_PU(ENCODER_CLK);
    PIN_input_PU(ENCODER_DT);
    PIN_input_PU(ENCODER_SW);

    // Enable shunt 0 by default
    PIN_output(SHUNT0_EN);
    PIN_output(SHUNT1_EN);
    PIN_output(SHUNT2_EN);
    PIN_high(SHUNT0_EN);
    PIN_low(SHUNT1_EN);
    PIN_low(SHUNT2_EN);

    initTimer();
    initBuzzer();

    INA219_init();
    OLED_init();
}

void main()
{
    __xdata char    buf[25];
    __xdata int8_t  recalibrate      = 0;
    __data uint32_t last_system_time = 0;
    __data int32_t  shunt_voltage_uV;
    __data int32_t  last_bus_voltage_mV = -1;
    __data int32_t  last_power_uW       = -1;
    __data int32_t  last_current_uA     = -1;
    __data int32_t  bus_voltage_mV;
    __data int32_t  power_uW;
    __data int32_t  current_uA;

    // Shunt resistor 0 = 0.1 Ω
    // Shunt resistor 1 =   1 Ω
    // Shunt resistor 2 =  10 Ω
    __data uint8_t shunt    = 0;  // Use small shunt resistor by default
    __bit          low_volt = 0;

    startup();

    OLED_clear();
    OLED_printxy(0, 0, "-----Power Meter-----");
    OLED_printxy(0, 1, "Voltage          V");
    OLED_printxy(0, 2, "Current          mA");
    OLED_printxy(0, 3, "Power            mW");

    playBuzzer(warningSound);

    while (1)
    {
        if (process_encoder())
        {
            // TODO: functions
            sprintf(buf, "enc = %3u", encoder_value);
            OLED_printxy(0, 7, buf);
        }

        if (millis() - last_system_time >= 50)
        {
            last_system_time = millis();

            shunt_voltage_uV = INA219_get_shunt_voltage_uV();
            bus_voltage_mV   = INA219_get_bus_voltage_mV();
            power_uW         = INA219_get_power_uW();
            current_uA       = INA219_get_current_uA();

            if (shunt_voltage_uV != 0 && current_uA == 0)
            {
                INA219_init();
                delay(100);
                recalibrate++;
                continue;
            }

            if (low_volt == 1 && bus_voltage_mV >= 1500)
            {
                low_volt = 0;
            }

            if (bus_voltage_mV < 1200 || current_uA < 0)
            {
                low_volt = 1;
                OLED_printxy(8, 1, "       -");
                OLED_printxy(8, 2, "       -");
                OLED_printxy(8, 3, "       -");
                last_bus_voltage_mV = -1;
                last_power_uW       = -1;
                last_current_uA     = -1;

                // Reset to shunt 0
                if (shunt != 0)
                {
                    PIN_high(SHUNT0_EN);
                    PIN_low(SHUNT1_EN);
                    PIN_low(SHUNT2_EN);
                    INA219_switch_shunt(0);
                    shunt = 0;
                }
            }

            if (!low_volt)
            {
                // Switch from shunt 0 to shunt 1 if current is <= 100 mA
                if (current_uA <= 100000 && shunt == 0)
                {
                    PIN_high(SHUNT1_EN);
                    PIN_low(SHUNT0_EN);
                    INA219_switch_shunt(1);
                    shunt = 1;
                    delay(100);  // Wait for INA219 to get new data
                }
                // Switch from shunt 1 to shunt 2 if current is <= 10 mA
                if (current_uA <= 10000 && shunt == 1)
                {
                    PIN_high(SHUNT2_EN);
                    PIN_low(SHUNT1_EN);
                    INA219_switch_shunt(2);
                    shunt = 2;
                    delay(100);  // Wait for INA219 to get new data
                }
                // Switch from shunt 1 to shunt 0 if current is > 105 mA
                if (current_uA > 105000 && shunt == 1)
                {
                    PIN_high(SHUNT0_EN);
                    PIN_low(SHUNT1_EN);
                    INA219_switch_shunt(0);
                    shunt = 0;
                    delay(100);  // Wait for INA219 to get new data
                }
                // Switch from shunt 2 to shunt 1 if current is > 15 mA
                if (current_uA > 15000 && shunt == 2)
                {
                    PIN_high(SHUNT1_EN);
                    PIN_low(SHUNT2_EN);
                    INA219_switch_shunt(1);
                    shunt = 1;
                    delay(100);  // Wait for INA219 to get new data
                }

                if (bus_voltage_mV != last_bus_voltage_mV)
                {
                    sprintf(buf, "%4lu.%03lu", bus_voltage_mV / 1000, bus_voltage_mV % 1000);
                    OLED_printxy(8, 1, buf);
                    last_bus_voltage_mV = bus_voltage_mV;
                }

                if (current_uA != last_current_uA)
                {
                    sprintf(buf, "%4ld.%03ld", current_uA / 1000, current_uA % 1000);
                    OLED_printxy(8, 2, buf);
                    last_current_uA = current_uA;
                }

                if (power_uW != last_power_uW)
                {
                    sprintf(buf, "%6ld.%1ld", power_uW / 1000, (power_uW / 100) % 10);
                    OLED_printxy(8, 3, buf);
                    last_power_uW = power_uW;
                }
            }

            sprintf(buf, "Recal=%d Shunt=%d", recalibrate, shunt);
            OLED_printxy(0, 6, buf);
        }
    }
}
