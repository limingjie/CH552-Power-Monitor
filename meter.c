#include "meter.h"

#include <ch554.h>
#include <gpio.h>
#include <ina219.h>
#include <oled.h>
#include <stdint.h>
#include <time.h>

#include <font_5x8.h>
#include <font_8x16.h>

__data uint8_t shunt        = 0;  // Use the smallest shunt resistor by default
__data uint8_t recalibrate  = 0;
__bit          undervoltage = 0;

__data int32_t shunt_voltage_uV = 0;
__data int32_t current_uA       = 0;
__data int32_t bus_voltage_mV   = 0;
__data int32_t power_uW         = 0;
__data int32_t max_current_uA   = 0;
__data int32_t min_current_uA   = 0x7FFFFFFF;

void meter_init()
{
    // Enable shunt 0 by default
    PIN_output(SHUNT0_EN);
    PIN_output(SHUNT1_EN);
    PIN_output(SHUNT2_EN);
    PIN_low(SHUNT0_EN);
    PIN_low(SHUNT1_EN);
    PIN_low(SHUNT2_EN);

    INA219_init();
    PIN_high(SHUNT0_EN);
    INA219_switch_shunt(0);
}

inline void meter_display()
{
    OLED_setFont(&OLED_FONT_8x16);
    OLED_setCursor(0, 120);
    OLED_print("V");
    OLED_setCursor(2, 112);
    OLED_print("mA");
    OLED_setCursor(4, 112);
    OLED_print("mW");
    OLED_setFont(&OLED_FONT_5x8);
    OLED_setCursor(6, 0);
    OLED_print("MAX");
    OLED_setCursor(6, 116);
    OLED_print("mA");
    OLED_setCursor(7, 0);
    OLED_print("MIN");
    OLED_setCursor(7, 116);
    OLED_print("mA");
}

void meter_switch_to_shunt(uint8_t to_shunt)
{
    INA219_switch_shunt(to_shunt);
    shunt = to_shunt;
    delay(100);  // Wait for INA219 to get new data
}

inline void meter_undervoltage_lockout()
{
    if (shunt != 0)
    {
        PIN_high(SHUNT0_EN);
        PIN_low(SHUNT1_EN);
        PIN_low(SHUNT2_EN);
        meter_switch_to_shunt(0);
    }

    OLED_setFont(&OLED_FONT_8x16);
    OLED_setCursor(0, 28);
    OLED_print("         -");
    OLED_setCursor(2, 28);
    OLED_print("         -");
    OLED_setCursor(4, 28);
    OLED_print("         -");
    OLED_setFont(&OLED_FONT_5x8);
    OLED_setCursor(6, 44);
    OLED_print("         -");
    OLED_setCursor(7, 44);
    OLED_print("         -");
}

inline __bit meter_check_calibration()
{
    // Sometimes, INA219 can return the shunt voltage but the current is zero,
    // it looks like the current LSB is missing, recalibrate it.
    if (current_uA == 0 && shunt_voltage_uV != 0)
    {
        INA219_init();
        delay(100);
        recalibrate++;
        return 1;
    }

    return 0;
}

inline void meter_check_undervoltage()
{
    if (undervoltage)
    {
        if (bus_voltage_mV >= 1500)
        {
            undervoltage = 0;
        }
    }
    else if (bus_voltage_mV < 1200 || current_uA < 0)
    {
        undervoltage = 1;
        meter_undervoltage_lockout();
    }
}

__bit meter_check_shunt()
{
    if (shunt == 0)
    {
        // Switch from shunt 0 to shunt 1 if current is <= 80 mA
        if (current_uA <= 80000)
        {
            PIN_high(SHUNT1_EN);
            PIN_low(SHUNT0_EN);
            meter_switch_to_shunt(1);

            return 1;
        }
    }
    else if (shunt == 1)
    {
        // Switch from shunt 1 to shunt 2 if current is <= 8 mA
        if (current_uA <= 8000)
        {
            PIN_high(SHUNT2_EN);
            PIN_low(SHUNT1_EN);
            meter_switch_to_shunt(2);

            return 1;
        }
        // Switch from shunt 1 to shunt 0 if current is > 100 mA
        else if (current_uA > 100000)
        {
            PIN_high(SHUNT0_EN);
            PIN_low(SHUNT1_EN);
            meter_switch_to_shunt(0);

            return 1;
        }
    }
    else  // shunt 2
    {
        // Switch from shunt 2 to shunt 1 if current is > 10 mA
        if (current_uA > 10000)
        {
            PIN_high(SHUNT1_EN);
            PIN_low(SHUNT2_EN);
            meter_switch_to_shunt(1);

            return 1;
        }
    }

    return 0;
}

void print_reading(uint8_t page, uint8_t font_width, int32_t value)
{
    __bit neg = 0;
    if (value < 0)
    {
        neg   = 1;
        value = -value;
    }

    // Remove the last digit
    value /= 10;

    // Print dot
    OLED_setCursor(page, 87);
    OLED_write('.');

    // Print fractional part
    uint8_t pos = 92 + font_width;
    uint8_t i;
    for (i = 2; i; i--)
    {
        OLED_setCursor(page, pos);
        OLED_write('0' + value % 10);
        value /= 10;
        pos -= font_width;
    }

    // Print integer part
    pos = 86 - font_width;
    i   = 7;
    while (value > 0)
    {
        OLED_setCursor(page, pos);
        OLED_write('0' + value % 10);
        value /= 10;
        pos -= font_width;
        i--;
    }

    if (i == 7)
    {
        OLED_setCursor(page, pos);
        OLED_write('0');
        pos -= font_width;
        i--;
    }

    // Print minus sign
    if (neg)
    {
        OLED_setCursor(page, pos);
        OLED_write('-');
        pos -= font_width;
        i--;
    }

    // Overwrite remaining digits
    for (; i; i--)
    {
        OLED_setCursor(page, pos);
        OLED_write(' ');
        pos -= font_width;
    }
}

void meter_run()
{
    shunt_voltage_uV = INA219_get_shunt_voltage_uV();
    current_uA       = INA219_get_current_uA();
    bus_voltage_mV   = INA219_get_bus_voltage_mV();
    power_uW         = INA219_get_power_uW();

    if (meter_check_calibration())
    {
        return;
    }

    // if (shunt_voltage_uV != last_shunt_voltage_uV)
    // {
    //     print_reading(7, 4, shunt_voltage_uV);
    //     last_shunt_voltage_uV = shunt_voltage_uV;
    // }

    meter_check_undervoltage();

    if (!undervoltage)
    {
        if (meter_check_shunt())  // Shunt changed
        {
            return;
        }

        if (current_uA > max_current_uA)
        {
            max_current_uA = current_uA;
        }

        if (current_uA < min_current_uA)
        {
            min_current_uA = current_uA;
        }

        OLED_setFont(&OLED_FONT_8x16);
        print_reading(0, 8, bus_voltage_mV);
        print_reading(2, 8, current_uA);
        print_reading(4, 8, power_uW);
        OLED_setFont(&OLED_FONT_5x8);
        print_reading(6, 6, max_current_uA);
        print_reading(7, 6, min_current_uA);
    }
}
