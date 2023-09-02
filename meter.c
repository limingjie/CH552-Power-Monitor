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

__code char str_lockout[] = "         -";

void meter_switch_to_shunt(uint8_t to_shunt)
{
    INA219_switch_shunt(to_shunt);
    shunt = to_shunt;

    OLED_setFont(&OLED_FONT_8x16);
    OLED_setColor(0);
    OLED_setCursor(0, 0);
    OLED_write('0' + shunt);
    OLED_setColor(1);

    delay(100);  // Wait for INA219 to get new data
}

void meter_reset()
{
    max_current_uA = 0;
    min_current_uA = 0x7FFFFFFF;
}

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
    meter_switch_to_shunt(0);
}

inline void meter_display()
{
    OLED_setFont(&OLED_FONT_8x16);
    OLED_setCursor(0, 120);
    OLED_write('V');
    OLED_setCursor(2, 120);
    OLED_write('A');
    OLED_setCursor(4, 120);
    OLED_write('W');
    OLED_setFont(&OLED_FONT_5x8);
    OLED_setCursor(6, 0);
    OLED_print("MAX");
    OLED_setCursor(6, 118);
    OLED_write('A');
    OLED_setCursor(7, 0);
    OLED_print("MIN");
    OLED_setCursor(7, 118);
    OLED_write('A');
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
    OLED_setCursor(0, 27);
    OLED_print(str_lockout);
    OLED_setCursor(2, 27);
    OLED_print(str_lockout);
    OLED_setCursor(4, 27);
    OLED_print(str_lockout);
    OLED_setFont(&OLED_FONT_5x8);
    OLED_setCursor(6, 47);
    OLED_print(str_lockout);
    OLED_setCursor(7, 47);
    OLED_print(str_lockout);
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

void meter_check_undervoltage()
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

// Print the reading and unit
// - Print the reading in proper unit, either V/A/W or mV/mA/mW.
//   - [0, 1000000)   ->  xxx.yy  mV/mA/mW
//   - [1000000, Max] -> xxxx.yyy V/A/W
// - Handling at most 10 digits including floating point and minus sign.
//   - xxxxxx.yyy
//   - -xxxxx.yyy
// - Right aligned and fill the remain digits with space ' '.
void print_reading(uint8_t page, uint8_t reading_column, uint8_t unit_column, int32_t reading)
{
    static char str[11];
    uint8_t     decimal_precision;
    uint8_t     digits = 10;
    __bit       neg    = 0;

    // Handle negative number
    if (reading < 0)
    {
        neg     = 1;
        reading = -reading;
    }

    // Calculate and print the unit of the reading
    if (reading < 1000)  // uV/uA/uW
    {
        OLED_setCursor(page, unit_column);
        OLED_write('u');
        decimal_precision = 0;
    }
    else if (reading < 1000000)  // mV/mA/mW
    {
        OLED_setCursor(page, unit_column);
        OLED_write('m');
        decimal_precision = 3;
    }
    else  // V/A/W
    {
        OLED_setCursor(page, unit_column);
        OLED_write(' ');
        decimal_precision = 3;
        reading /= 1000;
    }

    str[digits] = '\0';  // End of the string.

    // Calculate the fractional part
    if (decimal_precision)
    {
        while (decimal_precision)
        {
            str[--digits] = reading % 10 + '0';
            reading /= 10;
            --decimal_precision;
        }

        // Place the decimal point
        str[--digits] = '.';
    }

    // Calculate the integer part
    if (reading == 0)
    {
        str[--digits] = '0';
    }
    else
    {
        while (reading)
        {
            str[--digits] = reading % 10 + '0';
            reading /= 10;
        }
    }

    // Place the minus sign
    if (neg)
    {
        str[--digits] = '-';
    }

    // Fill in spaces
    while (digits)
    {
        str[--digits] = ' ';
    }

    OLED_setCursor(page, reading_column);
    OLED_print(str);
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
        print_reading(0, 27, 112, bus_voltage_mV * 1000);
        print_reading(2, 27, 112, current_uA);
        print_reading(4, 27, 112, power_uW);
        OLED_setFont(&OLED_FONT_5x8);
        print_reading(6, 47, 112, max_current_uA);
        print_reading(7, 47, 112, min_current_uA);
    }
}
