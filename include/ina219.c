//
// INA219 CH552 library
//
// A CH552 library to communicate with INA219
//
// Required Libraries
// - I2C library from https://github.com/wagiminator/CH552-USB-OLED
//
// References
// - https://github.com/Zanduino/INA
// - https://github.com/adafruit/Adafruit_INA219
// - https://github.com/shermannatrix/mikroC-Extra-Examples/tree/master/External%20Devices%20(I2C)/INA219
//
// Mar 2023 by Li Mingjie
// - Email:  limingjie@outlook.com
// - GitHub: https://github.com/limingjie/
//

#include "ina219.h"

#include <i2c.h>

// Shunt resistor 0 = 0.1 Ω
// Shunt resistor 1 =   1 Ω
// Shunt resistor 2 =  10 Ω
__data uint8_t        current_LSB    = INA219_CURRENT_LSB_uA_0;
__data uint16_t       power_LSB      = INA219_POWER_LSB_uW_0;
__code const uint8_t  current_LSBs[] = {INA219_CURRENT_LSB_uA_0, INA219_CURRENT_LSB_uA_1, INA219_CURRENT_LSB_uA_2};
__code const uint16_t power_LSBs[]   = {INA219_POWER_LSB_uW_0, INA219_POWER_LSB_uW_1, INA219_POWER_LSB_uW_2};

uint16_t INA219_read_word(uint8_t reg)
{
    uint16_t word;
    I2C_start(INA219_ADDR);
    I2C_write(reg);
    I2C_restart(INA219_ADDR | 1);
    word = (I2C_read(1) << 8) | I2C_read(0);
    I2C_stop();

    return word;
}

void INA219_write_word(uint8_t reg, uint16_t word)
{
    I2C_start(INA219_ADDR);
    I2C_write(reg);
    I2C_write((uint8_t)(word >> 8));    // send MSB first
    I2C_write((uint8_t)(word & 0xFF));  // send LSB
    I2C_stop();
}

void INA219_init()
{
    INA219_write_word(INA219_CONFIGURATION_REGISTER, INA219_CONFIG_32V_320mV_16AVG_CONTINUOUS);
    INA219_write_word(INA219_CALIBRATION_REGISTER, INA219_CALIBRATION);
}

uint16_t INA219_get_raw_shunt_voltage()
{
    return INA219_read_word(INA219_SHUNT_VOLTAGE_REGISTER);
}

uint16_t INA219_get_raw_bus_voltage()
{
    return INA219_read_word(INA219_BUS_VOLTAGE_REGISTER);
}

uint16_t INA219_get_raw_power()
{
    return INA219_read_word(INA219_POWER_REGISTER);
}

uint16_t INA219_get_raw_current()
{
    return INA219_read_word(INA219_CURRENT_REGISTER);
}

int32_t INA219_get_shunt_voltage_uV()
{
    return (int16_t)INA219_get_raw_shunt_voltage() * (int32_t)INA219_SHUNT_VOLTAGE_LSB_uV;
}

int32_t INA219_get_bus_voltage_mV()
{
    return (INA219_get_raw_bus_voltage() >> 3) * (uint32_t)INA219_BUS_VOLTAGE_LSB_mV;
}

int32_t INA219_get_power_uW()
{
    return (int32_t)INA219_get_raw_power() * power_LSB;
}

int32_t INA219_get_current_uA()
{
    return (int32_t)INA219_get_raw_current() * current_LSB;
}

void INA219_switch_shunt(uint8_t shunt)
{
    power_LSB   = power_LSBs[shunt];
    current_LSB = current_LSBs[shunt];
}
