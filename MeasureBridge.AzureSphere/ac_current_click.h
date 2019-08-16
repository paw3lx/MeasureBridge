#pragma once

#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdint.h>

int ReadACCurrentBytes(uint8_t* byte_1, uint8_t* byte_2);

float GetCurrentAC(void);