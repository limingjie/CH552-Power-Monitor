#pragma once

#include <stdint.h>

typedef struct OLED_font
{
    __code uint8_t* data;     // Font data
    uint8_t         width;    // Font width in pixels
    uint8_t         height;   // Font height in OLED pages (8 pixels per page)
    uint8_t         spacing;  // Character spacing
    uint8_t         first;    // The code point of the first character
} OLED_font;
