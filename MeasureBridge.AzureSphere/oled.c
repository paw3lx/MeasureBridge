#include "oled.h"
#include <math.h>

uint8_t oled_state = 0;

float ac_current;
float ac_averageLastHour;
float kWhToday;
float kWhLast7Days;
float kWhLastMonth;
char currentTimeBuffer[26];
bool clkBoardRelay1IsOn;

/**
  * @brief  OLED initialization.
  * @param  None.
  * @retval Positive if was unsuccefully, zero if was succefully.
  */
uint8_t oled_init()
{
	return sd1306_init();
}

void update_oled()
{
	switch (oled_state)
	{
		case 0:
			UpdateConsumption();
			break;
		case 1:
			UpdateProjections();
			break;
		case 2:
			ShowTime();
			break;
		case 3:
			ShowClickState();
			break;
	}
	
}

void UpdateOledState(void)
{
	if (oled_state < 3)
	{
		oled_state += 1;
		oled_state = oled_state % 3;
	}
	update_oled();
}

void UpdateConsumption(void)
{
	clear_oled_buffer();
	sd1306_draw_string(OLED_TITLE_X, OLED_TITLE_Y, "  Current", FONT_SIZE_TITLE, white_pixel);

	uint8_t ac_string_data[10];
	uint8_t str_label[] = "Current [A]: ";
	// Convert x value to string
	ftoa(ac_current, ac_string_data, 4);

	// Draw a label at line 1
	sd1306_draw_string(OLED_LINE_1_X, OLED_LINE_1_Y, str_label, FONT_SIZE_LINE, white_pixel);
	// Draw the value of x
	sd1306_draw_string(sizeof(str_label) * 6, OLED_LINE_1_Y, ac_string_data, FONT_SIZE_LINE, white_pixel);

	uint8_t watts_string_data[10];
	uint8_t str_label2[] = "Power   [W]: ";
	// Convert x value to string
	float wats = ac_current * 230.0;
	ftoa(wats, watts_string_data, 1);

	// Draw a label at line 1
	sd1306_draw_string(OLED_LINE_2_X, OLED_LINE_2_Y, str_label2, FONT_SIZE_LINE, white_pixel);
	// Draw the value of x
	sd1306_draw_string(sizeof(str_label2) * 6, OLED_LINE_2_Y, watts_string_data, FONT_SIZE_LINE, white_pixel);

	uint8_t str_label3[] = "Last h  [A]: ";
	uint8_t ac_last_hour_string_data[10];
	// Convert x value to string
	ftoa(ac_averageLastHour, ac_last_hour_string_data, 2);

	// Draw a label at line 1
	sd1306_draw_string(OLED_LINE_3_X, OLED_LINE_3_Y, str_label3, FONT_SIZE_LINE, white_pixel);
	// Draw the value of x
	sd1306_draw_string(sizeof(str_label3) * 6, OLED_LINE_3_Y, ac_last_hour_string_data, FONT_SIZE_LINE, white_pixel);

	sd1306_refresh();
}

void UpdateProjections(void)
{
	clear_oled_buffer();
	sd1306_draw_string(OLED_TITLE_X, OLED_TITLE_Y, "Consumption", FONT_SIZE_TITLE, white_pixel);

	uint8_t kwh_today_string_data[10];
	uint8_t str_label[] = "Today  [kWh]: ";
	// Convert x value to string
	ftoa(kWhToday, kwh_today_string_data, 4);

	// Draw a label at line 1
	sd1306_draw_string(OLED_LINE_1_X, OLED_LINE_1_Y, str_label, FONT_SIZE_LINE, white_pixel);
	// Draw the value of x
	sd1306_draw_string(sizeof(str_label) * 6, OLED_LINE_1_Y, kwh_today_string_data, FONT_SIZE_LINE, white_pixel);

	uint8_t kwh_7days_string_data[10];
	uint8_t str_label2[] = "7 days [kWh]: ";
	// Convert x value to string
	ftoa(kWhLast7Days, kwh_7days_string_data, 4);

	// Draw a label at line 1
	sd1306_draw_string(OLED_LINE_2_X, OLED_LINE_2_Y, str_label2, FONT_SIZE_LINE, white_pixel);
	// Draw the value of x
	sd1306_draw_string(sizeof(str_label2) * 6, OLED_LINE_2_Y, kwh_7days_string_data, FONT_SIZE_LINE, white_pixel);

	uint8_t str_label3[] = "Month  [kWh]: ";
	uint8_t kwh_last_month_string_data[10];
	// Convert x value to string
	ftoa(kWhLastMonth, kwh_last_month_string_data, 4);

	// Draw a label at line 1
	sd1306_draw_string(OLED_LINE_3_X, OLED_LINE_3_Y, str_label3, FONT_SIZE_LINE, white_pixel);
	// Draw the value of x
	sd1306_draw_string(sizeof(str_label3) * 6, OLED_LINE_3_Y, kwh_last_month_string_data, FONT_SIZE_LINE, white_pixel);

	sd1306_refresh();
}

void ShowClickState(void)
{
	clear_oled_buffer();
	sd1306_draw_string(OLED_TITLE_X, 16, " R E L A Y", FONT_SIZE_TITLE, white_pixel);
	if (clkBoardRelay1IsOn == true) {
		sd1306_draw_string(OLED_TITLE_X, 40, "    O N", FONT_SIZE_TITLE, white_pixel);
	}
	else {
		sd1306_draw_string(OLED_TITLE_X, 40, "   O F F", FONT_SIZE_TITLE, white_pixel);
	}
	sd1306_refresh();
}

void ShowTime(void)
{
	clear_oled_buffer();
	if (clkBoardRelay1IsOn == true) {
		sd1306_draw_string(OLED_TITLE_X, OLED_TITLE_Y, "  RELAY ON", FONT_SIZE_TITLE, white_pixel);
		uint8_t str_label2[] = "     ";
		char elapsedTime[11];
		strncpy(elapsedTime, elapsedTimeBuffer, 20);
		elapsedTime[20] = '\0'; // place the null terminator

		// Draw a label at line 1
		sd1306_draw_string(OLED_LINE_2_X, OLED_LINE_2_Y, str_label2, FONT_SIZE_LINE, white_pixel);
		// Draw the value of x
		sd1306_draw_string(sizeof(str_label2) * 6, OLED_LINE_2_Y, elapsedTime, FONT_SIZE_LINE, white_pixel);
	}
	else {
		sd1306_draw_string(OLED_TITLE_X, OLED_TITLE_Y, " RELAY OFF", FONT_SIZE_TITLE, white_pixel);
	}
	

	char timeToDisplay[21];
	strncpy(timeToDisplay, currentTimeBuffer, 20);
	timeToDisplay[20] = '\0'; // place the null terminator
	// Draw a label at line 1
	sd1306_draw_string(OLED_LINE_4_X, OLED_LINE_4_Y, timeToDisplay, FONT_SIZE_LINE, white_pixel);



	sd1306_refresh();
}

/**
  * @brief  Converts a given integer x to string uint8_t[]
  * @param  n: float number to convert
  * @param  res:
  * @param  afterpoint:
  * @retval None.
  */
void ftoa(float n, uint8_t* res, int32_t afterpoint)
{
	// Extract integer part 
	int32_t ipart = (int32_t)n;

	// Extract floating part 
	float fpart = n - (float)ipart;

	int32_t i;

	if (ipart < 0)
	{
		res[0] = '-';
		res++;
		ipart *= -1;
	}

	if (fpart < 0)
	{
		fpart *= -1;

		if (ipart == 0)
		{
			res[0] = '-';
			res++;
		}
	}

	// convert integer part to string 
	i = intToStr(ipart, res, 1);

	// check for display option after point 
	if (afterpoint != 0)
	{
		res[i] = '.';  // add dot 

		// Get the value of fraction part upto given no. 
		// of points after dot. The third parameter is needed 
		// to handle cases like 233.007 
		fpart = fpart * pow(10, afterpoint);

		intToStr((int32_t)fpart, res + i + 1, afterpoint);
	}
}

/**
  * @brief  Converts a given integer x to string uint8_t[]
  * @param  x: x integer input
  * @param  str: uint8_t array output
  * @param  d: Number of zeros added
  * @retval i: number of digits
  */
int32_t intToStr(int32_t x, uint8_t str[], int32_t d)
{
	int32_t i = 0;
	uint8_t flag_neg = 0;

	if (x < 0)
	{
		flag_neg = 1;
		x *= -1;
	}
	while (x)
	{
		str[i++] = (x % 10) + '0';
		x = x / 10;
	}

	// If number of digits required is more, then 
	// add 0s at the beginning 
	while (i < d)
	{
		str[i++] = '0';
	}

	if (flag_neg)
	{
		str[i] = '-';
		i++;
	}

	reverse(str, i);
	str[i] = '\0';
	return i;
}

// reverses a string 'str' of length 'len' 
static void reverse(uint8_t* str, int32_t len)
{
	int32_t i = 0;
	int32_t j = len - 1;
	int32_t temp;

	while (i < j)
	{
		temp = str[i];
		str[i] = str[j];
		str[j] = temp;
		i++; j--;
	}
}