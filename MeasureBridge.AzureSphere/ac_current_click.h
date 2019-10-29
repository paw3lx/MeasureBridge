#pragma once

#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdint.h>

int read_ac_current_bytes(uint8_t* byte_1, uint8_t* byte_2);
float get_current_adc(void);
float get_current_ac(uint8_t measurements);