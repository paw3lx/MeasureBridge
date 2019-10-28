#pragma once

#include <stdint.h>
#include "sd1306.h"

#define OLED_TITLE_X      0
#define OLED_TITLE_Y      0
#define OLED_LINE_1_X     0
#define OLED_LINE_1_Y     16
#define OLED_LINE_2_X     0
#define OLED_LINE_2_Y     26
#define OLED_LINE_3_X     0
#define OLED_LINE_3_Y     36
#define OLED_LINE_4_X     0
#define OLED_LINE_4_Y     46

#define FONT_SIZE_TITLE   2
#define FONT_SIZE_LINE    1

extern float ac_current; 
extern float ac_averageLastHour;
extern float kWhToday;
extern float kWhLast7Days;
extern float kWhLastMonth;
extern char currentTimeBuffer[26];
extern char elapsedTimeBuffer[20];

extern uint8_t oled_state;

extern uint8_t oled_init(void);

extern void update_oled(void);

void UpdateOledState(void);

void UpdateConsumption(void);
void UpdateProjections(void);
void ShowClickState(void);
void ShowTime(void);
/**
  * @brief  Converts a given integer x to string uint8_t[]
  * @param  n: float number to convert
  * @param  res:
  * @param  afterpoint:
  * @retval None.
  */
extern void ftoa(float n, uint8_t* res, int32_t afterpoint);
/**
  * @brief  Converts a given integer x to string uint8_t[]
  * @param  x: x integer input
  * @param  str: uint8_t array output
  * @param  d: Number of zeros added
  * @retval i: number of digits
  */
int32_t intToStr(int32_t x, uint8_t str[], int32_t d);

static void reverse(uint8_t* str, int32_t len);