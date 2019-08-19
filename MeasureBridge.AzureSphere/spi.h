#pragma once

#include "oled.h"

int InitSpi(void);

void closeSpi(void);

extern int spiFd;