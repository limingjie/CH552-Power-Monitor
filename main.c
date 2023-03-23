#include <stdio.h>  // sprintf

#include <ch554.h>
#include <system.h>  // mcu_config()
#include <time.h>    // millis(), delay()
#include <oled.h>    // OLED
#include <ina219.h>  // INA219
#include <gpio.h>
#include <buzzer.h>

// All key pins are pulled up.
// - A key is pressed if the input bit changes from 1 to 0.
// - A key is released if the input bit changes from 0 to 1.
// - A key is down if the input bit is 0.
// #define KEY_PRESSED(KEY)  ((lastKeyState & (1 << (KEY))) && !(P1 & (1 << (KEY))))
// #define KEY_RELEASED(KEY) (!(lastKeyState & (1 << (KEY))) && (P1 & (1 << (KEY))))
// #define KEY_DOWN(KEY)     (!(P1 & (1 << (KEY))))
// #define DEBOUNCE_INTERVAL 8  // 8 ms

#define SHUNT0_EN P30
#define SHUNT1_EN P31
#define SHUNT2_EN P32

#define ENCODER_CLK P11
#define ENCODER_DT  P33
#define ENCODER_SW  P34
uint8_t encoder_value = 0;

// __xdata const uint8_t warningSound[] = {3, C4, 2, C4, 2, C4, 2};
__xdata const uint8_t warningSound[] = {1, C4, 1};

// https://forum.arduino.cc/t/reading-rotary-encoders-as-a-state-machine/937388
uint8_t process_encoder()
{
    static uint8_t state    = 0;
    uint8_t        CLKstate = PIN_read(ENCODER_CLK);
    uint8_t        DTstate  = PIN_read(ENCODER_DT);
    switch (state)
    {
        case 0:  // Idle state, encoder not turning
            if (!CLKstate)
            {    // Turn clockwise and CLK goes low first
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

    // Shunt selection pins
    // SHUNT0_EN - Shunt 0
    // SHUNT1_EN - Shunt 1
    PIN_output(SHUNT0_EN);
    PIN_high(SHUNT0_EN);
    PIN_output(SHUNT1_EN);
    PIN_low(SHUNT1_EN);
    PIN_output(SHUNT2_EN);
    PIN_low(SHUNT2_EN);

    initTimer();

    INA219_init();
    OLED_init();

    initBuzzer();
}

void main()
{
    __xdata char     buf[25];
    __xdata int8_t   recalibrate      = 0;
    __xdata uint32_t count            = 0;
    uint32_t         last_system_time = 0;

    // int32_t  last_shunt_voltage_uV = -1;
    int32_t last_bus_voltage_mV = -1;
    int32_t last_power_uW       = -1;
    int32_t last_current_uA     = -1;
    int32_t shunt_voltage_uV;
    int32_t bus_voltage_mV;
    int32_t power_uW;
    int32_t current_uA;

    // Shunt resistor 0 = 0.1 Ω
    // Shunt resistor 1 =  10 Ω
    uint8_t shunt    = 0;  // Use small shunt resistor by default
    __bit   low_volt = 0;  // Use small shunt resistor by default

    startup();

    OLED_clear();
    OLED_printxy(0, 0, "-----Power Meter-----");
    OLED_printxy(0, 1, "Voltage          V");
    OLED_printxy(0, 2, "Current          mA");
    OLED_printxy(0, 3, "Power            mW");
    // OLED_printxy(0, 4, "Shunt            mV");
    // OLED_printxy(0, 5, "Eq.Res           Ohm");

    playBuzzer(warningSound);

    while (1)
    {
        if (process_encoder())
        {
            sprintf(buf, "enc = %3u", encoder_value);
            OLED_printxy(0, 7, buf);
        }

        if (millis() - last_system_time >= 50)
        {
            last_system_time = millis();

            // uint16_t raw_shunt        = INA219_get_raw_shunt_voltage();
            // uint16_t raw_bus_voltage  = INA219_get_raw_bus_voltage();
            // uint16_t raw_power        = INA219_get_raw_power();
            // uint16_t raw_current      = INA219_get_raw_current();
            shunt_voltage_uV = INA219_get_shunt_voltage_uV();
            bus_voltage_mV   = INA219_get_bus_voltage_mV();
            power_uW         = INA219_get_power_uW();
            current_uA       = INA219_get_current_uA();

            if (shunt_voltage_uV != 0 && current_uA == 0)
            {
                delay(100);
                INA219_init();
                recalibrate++;
                continue;
            }

            if (low_volt == 1 && bus_voltage_mV >= 1200)
            {
                low_volt = 0;
            }

            if (bus_voltage_mV < 1000 || current_uA < 0)
            {
                low_volt = 1;
                OLED_printxy(8, 1, "       -");
                OLED_printxy(8, 2, "       -");
                OLED_printxy(8, 3, "       -");
                // OLED_printxy(8, 4, "       -");
                // OLED_printxy(8, 5, "       -");
                // last_shunt_voltage_uV = -1;
                last_bus_voltage_mV = -1;
                last_power_uW       = -1;
                last_current_uA     = -1;

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
                // Switch from shunt 0 to shunt 1 if current is less than 20 mA
                if (current_uA <= 20000 && shunt == 0)
                {
                    PIN_high(SHUNT1_EN);
                    PIN_low(SHUNT0_EN);
                    INA219_switch_shunt(1);
                    shunt = 1;
                    delay(100);  // Wait for INA219 to get new data
                }
                // Switch from shunt 1 to shunt 2 if current is less than 10 mA
                if (current_uA <= 10000 && shunt == 1)
                {
                    PIN_high(SHUNT2_EN);
                    PIN_low(SHUNT1_EN);
                    INA219_switch_shunt(2);
                    shunt = 2;
                    delay(100);  // Wait for INA219 to get new data
                }
                // Switch from shunt 1 to shunt 0 if current is greater than 25 mA
                if (current_uA > 25000 && shunt == 1)
                {
                    PIN_high(SHUNT0_EN);
                    PIN_low(SHUNT1_EN);
                    INA219_switch_shunt(0);
                    shunt = 0;
                    delay(100);  // Wait for INA219 to get new data
                }
                // Switch from shunt 2 to shunt 1 if current is greater than 15 mA
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

                // if (shunt_voltage_uV != last_shunt_voltage_uV)
                // {
                //     sprintf(buf, "%4ld.%03ld", shunt_voltage_uV / 1000, shunt_voltage_uV % 1000);
                //     OLED_printxy(8, 4, buf);
                //     last_shunt_voltage_uV = shunt_voltage_uV;
                // }

                // if (current_uA != 0)
                // {
                //     int32_t eqr_mOhm = (float)bus_voltage_mV * 1e6 / (float)current_uA;
                //     sprintf(buf, "%7ld.%01ld", eqr_mOhm / 1000, (eqr_mOhm / 100) % 10);
                //     OLED_printxy(7, 5, buf);
                // }
                // else
                // {
                //     OLED_printxy(7, 5, "        -");
                // }
            }

            sprintf(buf, "Recal=%d Shunt=%d", recalibrate, shunt);
            OLED_printxy(0, 6, buf);
            // sprintf(buf, "FRM=%lu FPS=%03lu", count++, count * 1000 / millis());
        }
    }
}

// Benchmark
// uint16_t a = 0xcccc, b = 0xaaaa, c = 0x5555;
// uint8_t  d = 0x66, e = 0x77;
// sprintf(buf, "%05u%05u%05u%03u%03u", a++, b++, c++, d++, e++);
// OLED_printxy(0, 0, buf);
// sprintf(buf, "%05u%05u%05u%03u%03u", a++, b++, c++, d++, e++);
// OLED_printxy(0, 1, buf);
// sprintf(buf, "%05u%05u%05u%03u%03u", a++, b++, c++, d++, e++);
// OLED_printxy(0, 2, buf);
// sprintf(buf, "%05u%05u%05u%03u%03u", a++, b++, c++, d++, e++);
// OLED_printxy(0, 3, buf);
// sprintf(buf, "%05u%05u%05u%03u%03u", a++, b++, c++, d++, e++);
// OLED_printxy(0, 4, buf);
// sprintf(buf, "%05u%05u%05u%03u%03u", a++, b++, c++, d++, e++);
// OLED_printxy(0, 5, buf);
// sprintf(buf, "%05u%05u%05u%03u%03u", a++, b++, c++, d++, e++);
// OLED_printxy(0, 6, buf);
