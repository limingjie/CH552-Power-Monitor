#include "meter.h"

#include <ch554.h>
#include <gpio.h>
#include <ina219.h>
#include <oled.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

__data uint8_t shunt        = 0;  // Use the smallest shunt resistor by default
__bit          undervoltage = 0;
__data int8_t  recalibrate  = 0;

__xdata int32_t shunt_voltage_uV      = 0;
__xdata int32_t current_uA            = 0;
__xdata int32_t bus_voltage_mV        = 0;
__xdata int32_t power_uW              = 0;
__xdata int32_t last_shunt_voltage_uV = -1;
__xdata int32_t last_current_uA       = -1;
__xdata int32_t last_bus_voltage_mV   = -1;
__xdata int32_t last_power_uW         = -1;

__xdata char buf[25];

void meter_init()
{
    // Enable shunt 0 by default
    PIN_output(SHUNT0_EN);
    PIN_output(SHUNT1_EN);
    PIN_output(SHUNT2_EN);
    PIN_high(SHUNT0_EN);
    PIN_low(SHUNT1_EN);
    PIN_low(SHUNT2_EN);

    INA219_init();
}

inline void meter_display()
{
    OLED_clear();
    OLED_printxy(0, 0, "-----Power Meter-----");
    OLED_printxy(0, 1, "Voltage            V");
    OLED_printxy(0, 2, "Current            mA");
    OLED_printxy(0, 3, "Power              mW");
    OLED_printxy(0, 4, "Shunt              mV");
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

    last_shunt_voltage_uV = -1;
    last_bus_voltage_mV   = -1;
    last_power_uW         = -1;
    last_current_uA       = -1;

    OLED_printxy(8, 1, "         -");
    OLED_printxy(8, 2, "         -");
    OLED_printxy(8, 3, "         -");
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

inline void meter_check_shunt()
{
    if (shunt == 0)
    {
        // Switch from shunt 0 to shunt 1 if current is <= 80 mA
        if (current_uA <= 80000)
        {
            PIN_high(SHUNT1_EN);
            PIN_low(SHUNT0_EN);
            meter_switch_to_shunt(1);
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
        }
        // Switch from shunt 1 to shunt 0 if current is > 100 mA
        else if (current_uA > 100000)
        {
            PIN_high(SHUNT0_EN);
            PIN_low(SHUNT1_EN);
            meter_switch_to_shunt(0);
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
        }
    }
}

void print_thousands(uint8_t x, uint8_t y, __xdata int32_t value)
{
    sprintf(buf, "%6ld.%03ld", value / 1000, ((value < 0) ? -value : value) % 1000);
    OLED_printxy(x, y, buf);
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

    if (shunt_voltage_uV != last_shunt_voltage_uV)
    {
        print_thousands(8, 4, shunt_voltage_uV);
        last_shunt_voltage_uV = shunt_voltage_uV;
    }

    meter_check_undervoltage();

    if (!undervoltage)
    {
        meter_check_shunt();

        if (bus_voltage_mV != last_bus_voltage_mV)
        {
            print_thousands(8, 1, bus_voltage_mV);
            last_bus_voltage_mV = bus_voltage_mV;
        }

        if (current_uA != last_current_uA)
        {
            print_thousands(8, 2, current_uA);
            last_current_uA = current_uA;
        }

        if (power_uW != last_power_uW)
        {
            print_thousands(8, 3, power_uW);
            last_power_uW = power_uW;
        }
    }

    sprintf(buf, "Recal=%d Shunt=%d", recalibrate, shunt);
    OLED_printxy(0, 6, buf);
}
