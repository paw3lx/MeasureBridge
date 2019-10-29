#pragma once

#include "oled.h"

int init_spi(void);

void close_spi(void);

extern int spiFd;