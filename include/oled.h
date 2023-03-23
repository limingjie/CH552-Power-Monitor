//
// SSD1306 128x64 OLED library
//
// Required Libraries
// - I2C library from https://github.com/wagiminator/CH552-USB-OLED
//
// References
// - https://github.com/wagiminator/CH552-USB-OLED
//
// Mar 2023 by Li Mingjie
// - Email:  limingjie@outlook.com
// - GitHub: https://github.com/limingjie/
//

#pragma once
#include <stdint.h>

void OLED_init(void);          // OLED init function
void OLED_clear(void);         // OLED clear screen
void OLED_write(char c);       // OLED write a character or handle control characters
void OLED_print(char* str);    // OLED print string
void OLED_println(char* str);  // OLED print string with newline
void OLED_setline(uint8_t line);
void OLED_clearline(uint8_t line);
void OLED_printxy(uint8_t x, uint8_t y, char* str);
