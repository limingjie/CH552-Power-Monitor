//
// INA219 CH552 library
//
// A simple library to communicate with INA219
//
// Required Libraries
// - I2C from https://github.com/wagiminator/CH552-USB-OLED
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

#pragma once

#include <stdint.h>

// INA219 Registers
#define INA219_CONFIGURATION_REGISTER 0x00  // Read-Write
#define INA219_SHUNT_VOLTAGE_REGISTER 0x01  // Read-Only
#define INA219_BUS_VOLTAGE_REGISTER   0x02  // Read-Only
#define INA219_POWER_REGISTER         0x03  // Read-Only
#define INA219_CURRENT_REGISTER       0x04  // Read-Only
#define INA219_CALIBRATION_REGISTER   0x05  // Read-Write

// INA219 Configuration Register
#define INA219_CONFIG_RESET                 0x8000
#define INA219_CONFIG_BRGN_16V              0x0000  // Bus voltage range 16V
#define INA219_CONFIG_BRGN_32V              0x2000  // Bus voltage range 32V, default value
#define INA219_CONFIG_PG_1_40mV             0x0000  // Gain /1 -  ±40mV
#define INA219_CONFIG_PG_2_80mV             0x0800  // Gain /2 -  ±80mV
#define INA219_CONFIG_PG_4_160mV            0x1000  // Gain /4 - ±160mV
#define INA219_CONFIG_PG_8_320mV            0x1800  // Gain /8 - ±320mV
#define INA219_CONFIG_BADC_9BIT             0x0000  // 84uS
#define INA219_CONFIG_BADC_10BIT            0x0080  // 148uS
#define INA219_CONFIG_BADC_11BIT            0x0100  // 276uS
#define INA219_CONFIG_BADC_12BIT            0x0180  // 532uS
#define INA219_CONFIG_BADC_AVG_2            0x0480  // 1060uS
#define INA219_CONFIG_BADC_AVG_4            0x0500  // 2130uS
#define INA219_CONFIG_BADC_AVG_8            0x0580  // 4260uS
#define INA219_CONFIG_BADC_AVG_16           0x0600  // 8510uS
#define INA219_CONFIG_BADC_AVG_32           0x0680  // 17020uS
#define INA219_CONFIG_BADC_AVG_64           0x0700  // 34050uS
#define INA219_CONFIG_BADC_AVG_128          0x0780  // 68100uS
#define INA219_CONFIG_SADC_9BIT             0x0000  // 84uS
#define INA219_CONFIG_SADC_10BIT            0x0008  // 148uS
#define INA219_CONFIG_SADC_11BIT            0x0010  // 276uS
#define INA219_CONFIG_SADC_12BIT            0x0018  // 532uS
#define INA219_CONFIG_SADC_AVG_2            0x0048  // 1060uS
#define INA219_CONFIG_SADC_AVG_4            0x0050  // 2130uS
#define INA219_CONFIG_SADC_AVG_8            0x0058  // 4260uS
#define INA219_CONFIG_SADC_AVG_16           0x0060  // 8510uS
#define INA219_CONFIG_SADC_AVG_32           0x0068  // 17020uS
#define INA219_CONFIG_SADC_AVG_64           0x0070  // 34050uS
#define INA219_CONFIG_SADC_AVG_128          0x0078  // 68100uS
#define INA219_CONFIG_MODE_POWER_DOWN       0x00
#define INA219_CONFIG_MODE_SHUNT_TRIGGERED  0x01
#define INA219_CONFIG_MODE_BUS_TRIGGERED    0x02
#define INA219_CONFIG_MODE_BOTH_TRIGGERED   0x03
#define INA219_CONFIG_MODE_ADC_OFF          0x04
#define INA219_CONFIG_MODE_SHUNT_CONTINUOUS 0x05
#define INA219_CONFIG_MODE_BUS_CONTINUOUS   0x06
#define INA219_CONFIG_MODE_BOTH_CONTINUOUS  0x07

// Bus Voltage Register Flags
#define INA219_BUS_VOLTAGE_CONVERSION_READY 0x02
#define INA219_BUS_VOLTAGE_MATH_OVERFLOW    0x01

// Configuration
#define INA219_CONFIG_32V_320mV_16AVG_CONTINUOUS                                                                \
    INA219_CONFIG_BRGN_32V | INA219_CONFIG_PG_8_320mV | INA219_CONFIG_BADC_AVG_16 | INA219_CONFIG_SADC_AVG_16 | \
        INA219_CONFIG_MODE_BOTH_CONTINUOUS

// LSBs
#define INA219_SHUNT_VOLTAGE_LSB_uV 10
#define INA219_BUS_VOLTAGE_LSB_mV   4

// Calibration
//
// - Bus Voltage   =  32 V                    Bus_LSB     =  4 mV
// - Shunt Voltage = 320 mV (PGA = /8)        Shunt_LSB   = 10 uV
// - Max Current   =   2 A                    Current_LSB = ?
//
// 1. Choose Shunt Resistor
//    Max Shunt Resistor = 320 mV / 2 A = 0.16 Ω
//    Shunt Resistor Power Rating 2 A x 2 A x 0.16 Ω = 0.64 W
//    Choose 0.1 Ω to leave some buffer.
//
// 2. Calculate Current LSB
//    The current register's range is [-2^15, 2^15 - 1]
//    Minimal Current_LSB = 2 A / (2 ^ 15 - 1) = 0.000061037 ~ 61 uA
//    Round up to 100 uA
//
#define INA219_CURRENT_LSB_uA_0 100  // 0.1 Ω shunt resistor
#define INA219_CURRENT_LSB_uA_1 10   //   1 Ω shunt resistor
#define INA219_CURRENT_LSB_uA_2 1    //  10 Ω shunt resistor
//
// 3. Calculate the Calibration Register
//    Calibration_Register = 0.04096 / (Current_LSB x Rshunt) = 4096 = 0x1000
//    To better understand Calibration Register:
//    Current_Register     = Shunt_Register x Calibration_Register / 4096    (1) From Data Sheet
//    Current              = Current_Register x Current_LSB                  (2)
//    Current              = Shunt_Voltage / Rshunt                          (3)
//    Shunt_Voltage        = Shunt_Register x Shunt_LSB                      (4)
//    Current_Register     = Current / Current_LSB                           (2)+(3)+(4)
//                         = Shunt_Voltage / (Rshunt x Current_LSB)
//                            Shunt_Register x Shunt_LSB
//                         = ----------------------------                    (5)
//                               Current_LSB x Rshunt
//    Calibration_Register = 4096 x Shunt_LSB / (Current_LSB x Rshunt)       (1)+(5)
//                         = 4096 x 0.00001 / (Current_LSB x Rshunt)
//                         = 0.04096 / (Current_LSB x Rshunt)
//    The factor 4096 is too normalize Calibration_Register in the 12-bit ADC range.
//
#define INA219_CALIBRATION 0x1000
//
// 4. Calculate the Power Register LSB
//    Power_Register = (Bus_Voltage_Register x Current_Register) / 5000      (1) From Data Sheet
//    Power          = Power_Register x Power_LSB                            (2)
//    Power          = Bus_Voltage x Current                                 (3)
//    Power_LSB      = Power / Power_Register                                (1)+(2)+(3)
//                                Bus_Voltage x Current
//                   = --------------------------------------------------
//                      (Bus_Voltage_Register x Current_Register) / 5000
//                   = 5000 x Bus_LSB x Current_LSB
//                   = 5000 x 0.004 x Current_LSB
//    Power_LSB      = 20 x Current_LSB
//
#define INA219_POWER_LSB_uW_0 2000  // 20 x INA219_CURRENT_LSB_uA_0
#define INA219_POWER_LSB_uW_1 200   // 20 x INA219_CURRENT_LSB_uA_1
#define INA219_POWER_LSB_uW_2 20    // 20 x INA219_CURRENT_LSB_uA_2
//
// 5. Max Current in This Configuration
//    Current_1 = Max_Shunt_Voltage / Rshunt = 320 mV / 0.1 Ω = 3.2 A
//    Current_2 = Max_Current_Register x Current_LSB = (2^15 - 1) x 100 uA = 3.2767 A
//    Max Current = 3.2A
//
// 6. Resistor Rating
//    3.2 A x 3.2 A x 0.1 Ω = 1.024 W
//

void INA219_init(uint8_t addr);

uint16_t INA219_get_raw_shunt_voltage(uint8_t addr);
uint16_t INA219_get_raw_bus_voltage(uint8_t addr);
uint16_t INA219_get_raw_power(uint8_t addr);
uint16_t INA219_get_raw_current(uint8_t addr);

int32_t INA219_get_shunt_voltage_uV(uint8_t addr);
int32_t INA219_get_bus_voltage_mV(uint8_t addr);
int32_t INA219_get_power_uW(uint8_t addr);
int32_t INA219_get_current_uA(uint8_t addr);

void INA219_switch_shunt(uint8_t shunt);
