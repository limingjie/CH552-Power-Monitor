//
// SSD1306 128x64 OLED library
//
// Required Libraries
// - I2C library from https://github.com/wagiminator/CH552-USB-OLED
//
// References
// - https://github.com/wagiminator/CH552-USB-OLED
// - https://github.com/datacute/Tiny4kOLED
//
// Apr 2023 by Li Mingjie
// - Email:  limingjie@outlook.com
// - GitHub: https://github.com/limingjie/
//

#include "oled.h"

#include "i2c.h"

// OLED definitions
#define OLED_ADDR      0x78  // OLED write address (0x3C << 1)
#define OLED_CMD_MODE  0x00  // set command mode
#define OLED_DATA_MODE 0x40  // set data mode

// OLED commands
#define OLED_COLUMN_LOW         0x00  // set lower 4 bits of start column (0x00 - 0x0F)
#define OLED_COLUMN_HIGH        0x10  // set higher 4 bits of start column (0x10 - 0x1F)
#define OLED_MEM_ADDR_MODE      0x20  // Set memory addressing mode (following byte)
#define OLED_MEM_ADDR_MODE_PAGE 0x02  // Page addressing mode
#define OLED_MEM_ADDR_MODE_H    0x00  // Horizontal addressing mode
#define OLED_MEM_ADDR_MODE_V    0x01  // Vertical addressing mode
#define OLED_COLUMN_ADDRESSES   0x21  // set start and end column (following 2 bytes)
#define OLED_PAGE_ADDRESSES     0x22  // set start and end page (following 2 bytes)
#define OLED_STARTLINE          0x40  // set display start line (0x40-0x7F = 0-63)
#define OLED_CONTRAST           0x81  // Set display contrast (following a byte, 0-255) - (Reset 0x7F)
#define OLED_CHARGE_PUMP        0x8D  // (following byte - 0x14:enable, 0x10: disable)
#define OLED_H_FLIP_OFF         0xA0  // don't flip display horizontally
#define OLED_H_FLIP_ON          0xA1  // flip display horizontally
#define OLEF_RESUME_DISPLAY     0xA4  // Resume to RAM content display
#define OLEF_ENTIRE_DISPLAY_ON  0xA5  // Entire display ON
#define OLEF_NORMAL_DISPLAY     0xA6  // Normal display - (Reset)
#define OLED_INVERT_DISPLAY     0xA7  // Inverse display
#define OLED_MULTIPLEX          0xA8  // set multiplex ratio (following byte)
#define OLED_DISPLAY_OFF        0xAE  // Display OFF (sleep mode) - (Reset)
#define OLED_DISPLAY_ON         0xAF  // Display ON in normal mode
#define OLED_PAGE               0xB0  // set start page (following byte)
#define OLED_V_FLIP_OFF         0xC0  // don't flip display vertically
#define OLED_V_FLIP_ON          0xC8  // flip display vertically
#define OLED_OFFSET             0xD3  // set display offset (y-scroll: following byte)
#define OLED_COM_PINS           0xDA  // set COM pin config (following byte)

// OLED initialization sequence
__code uint8_t OLED_INIT_CMD[] = {
    OLED_MULTIPLEX,     0x3F,                  // set multiplex ratio
    OLED_CHARGE_PUMP,   0x14,                  // set DC-DC enable
    OLED_MEM_ADDR_MODE, OLED_MEM_ADDR_MODE_V,  // set page addressing mode
    OLED_COM_PINS,      0x12,                  // set com pins
    OLED_CONTRAST,      0x3F,                  // set contrast 63 / 255
    OLED_DISPLAY_ON                            // display on
};

__data uint8_t _page   = 0;  // OLED memory pages, from 0 to 7.
__data uint8_t _column = 0;  // OLED memory columns, from 0 to 127.
OLED_font*     _font;        // Default font

// OLED init function
void OLED_init(void)
{
    I2C_init();                // initialize I2C first
    I2C_start(OLED_ADDR);      // start transmission to OLED
    I2C_write(OLED_CMD_MODE);  // set command mode
    uint8_t size = sizeof(OLED_INIT_CMD);
    for (uint8_t i = 0; i < size; i++)
    {
        I2C_write(OLED_INIT_CMD[i]);  // send the command bytes
    }
    I2C_stop();  // stop transmission
}

// Set memory address range
void OLED_setMemoryAddress(uint8_t start_page, uint8_t end_page, uint8_t start_column, uint8_t end_column)
{
    _page   = start_page;
    _column = start_column;
    I2C_start(OLED_ADDR);              // start transmission to OLED
    I2C_write(OLED_CMD_MODE);          // set command mode
    I2C_write(OLED_PAGE_ADDRESSES);    // set page addresses
    I2C_write(start_page);             // set start page
    I2C_write(end_page);               // set end page
    I2C_write(OLED_COLUMN_ADDRESSES);  // set page addresses
    I2C_write(start_column);           // set start page
    I2C_write(end_column);             // set end page
    I2C_stop();                        // stop transmission
}

// Clear screen
void OLED_clear(void)
{
    OLED_setMemoryAddress(0, 7, 0, 127);

    I2C_start(OLED_ADDR);       // start transmission to OLED
    I2C_write(OLED_DATA_MODE);  // set data mode
    for (uint8_t page = 8; page; page--)
    {
        for (uint8_t column = 128; column; column--)
        {
            I2C_write(0x00);
        }
    }
    I2C_stop();  // stop transmission

    _page   = 0;
    _column = 0;
}

void OLED_setFont(OLED_font* font)
{
    _font = font;
}

void OLED_setCursor(uint8_t page, uint8_t column)
{
    _page   = page;
    _column = column;
}

// Helper
void OLED_plotChar(char c)
{
    uint8_t i, j;

    __code uint8_t* data = &_font->data[(c - _font->first) * _font->width * _font->height];

    for (i = _font->width; i; i--)
    {
        for (j = _font->height; j; j--)
        {
            I2C_write(*data++);
        }
        _column++;
    }
    for (i = _font->spacing; i; i--)
    {
        for (j = _font->height; j; j--)
        {
            I2C_write(0x00);
        }
        _column++;
    }
}

// Print a single character
void OLED_write(char c)
{
    OLED_setMemoryAddress(_page, _page + _font->height - 1, _column, 127);
    I2C_start(OLED_ADDR);       // start transmission to OLED
    I2C_write(OLED_DATA_MODE);  // set data mode
    OLED_plotChar(c);
    I2C_stop();  // stop transmission
}

void OLED_print(const char* str)
{
    OLED_setMemoryAddress(_page, _page + _font->height - 1, _column, 127);
    I2C_start(OLED_ADDR);       // start transmission to OLED
    I2C_write(OLED_DATA_MODE);  // set data mode
    while (*str)
    {
        OLED_plotChar(*str++);
    }
    I2C_stop();  // stop transmission
}
