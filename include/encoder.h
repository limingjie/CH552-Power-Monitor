#pragma once

#include <stdint.h>

void    encoder_init();
__bit   encoder_process();
uint8_t encoder_get_delta();
