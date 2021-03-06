/*
	This source code comes from Git repository
	https://github.com/CloudConnectKits/Azure_Sphere_SK_ADC_RTApp
*/
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"

#include <applibs/log.h>
#include <applibs/i2c.h>
#include <applibs/gpio.h>

#include "mt3620.h"
#include "deviceTwin.h"
#include "azure_iot_utilities.h"
#include "parson.h"
#include "i2c.h"
#include "time_helper.h"

extern bool relay_1_is_on;
extern bool relay_2_is_on;

extern int clickSocket1Relay1Fd;
extern int clickSocket1Relay2Fd;
extern int epollFd;

extern volatile sig_atomic_t terminationRequired;

static const char cstrDeviceTwinJsonInteger[] = "{\"%s\": %d}";
static const char cstrDeviceTwinJsonFloat[] = "{\"%s\": %.2f}";
static const char cstrDeviceTwinJsonBool[] = "{\"%s\": %s}";
static const char cstrDeviceTwinJsonString[] = "{\"%s\": \"%s\"}";

static int desiredVersion = 0;

float ac_averageLastHour = 0.0;
float kwh_today = 0.0;
float kwh_last_7_days = 0.0;
float kwh_last_month = 0.0;

int singleTimerFd = -1;

// Define each device twin key that we plan to catch, process, and send reported property for.
// .twinKey - The JSON Key piece of the key: value pair
// .twinVar - The address of the application variable keep this key: value pair data
// .twinFD - The associated File Descriptor for this item.  This is usually a GPIO FD.  NULL if NA.
// .twinGPIO - The associted GPIO number for this item.  NO_GPIO_ASSOCIATED_WITH_TWIN if NA
// .twinType - The data type for this item, TYPE_BOOL, TYPE_STRING, TYPE_INT, or TYPE_FLOAT
// .active_high - true if GPIO item is active high, false if active low.  This is used to init the GPIO 
twin_t twinArray[] = {
	{.twinKey = "clickBoardRelay1",.twinVar = &relay_1_is_on,.twinFd = &clickSocket1Relay1Fd,.twinGPIO = MT3620_GPIO1,.twinType = TYPE_BOOL,.active_high = true},
	{.twinKey = "clickBoardRelay2",.twinVar = &relay_2_is_on,.twinFd = &clickSocket1Relay2Fd,.twinGPIO = MT3620_GPIO43,.twinType = TYPE_BOOL,.active_high = true},
	{.twinKey = "acAverageLastHour",.twinVar = &ac_averageLastHour,.twinFd = NULL,.twinGPIO = NO_GPIO_ASSOCIATED_WITH_TWIN,.twinType = TYPE_FLOAT,.active_high = true},
	{.twinKey = "kWhToday",.twinVar = &kwh_today,.twinFd = NULL,.twinGPIO = NO_GPIO_ASSOCIATED_WITH_TWIN,.twinType = TYPE_FLOAT,.active_high = true},
	{.twinKey = "kWhLast7Days",.twinVar = &kwh_last_7_days,.twinFd = NULL,.twinGPIO = NO_GPIO_ASSOCIATED_WITH_TWIN,.twinType = TYPE_FLOAT,.active_high = true},
	{.twinKey = "kWhLastMonth",.twinVar = &kwh_last_month,.twinFd = NULL,.twinGPIO = NO_GPIO_ASSOCIATED_WITH_TWIN,.twinType = TYPE_FLOAT,.active_high = true}
};

// Calculate how many twin_t items are in the array.  We use this to iterate through the structure.
int twinArraySize = sizeof(twinArray) / sizeof(twin_t);

///<summary>
///		check to see if any of the device twin properties have been updated.  If so, send up the current data.
///</summary>
void checkAndUpdateDeviceTwin(char* property, void* value, data_type_t type, bool ioTCentralFormat)
{
	int nJsonLength = -1;

	char* pjsonBuffer = (char*)malloc(JSON_BUFFER_SIZE);
	if (pjsonBuffer == NULL) {
		Log_Debug("ERROR: not enough memory to report device twin changes.");
	}

	if (property != NULL) {

		// report current device twin data as reported properties to IoTHub

		switch (type) {
		case TYPE_BOOL:
			nJsonLength = snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinJsonBool, property, *(bool*)value ? "true" : "false", desiredVersion);
			break;
		case TYPE_FLOAT:
			nJsonLength = snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinJsonFloat, property, *(float*)value, desiredVersion);
			break;
		case TYPE_INT:
			nJsonLength = snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinJsonInteger, property, *(int*)value, desiredVersion);
			break;
		case TYPE_STRING:
			nJsonLength = snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrDeviceTwinJsonString, property, (char*)value, desiredVersion);
			break;
		}

		if (nJsonLength > 0) {
			Log_Debug("[MCU] Updating device twin: %s\n", pjsonBuffer);
			AzureIoT_TwinReportStateJson(pjsonBuffer, (size_t)nJsonLength);
		}
		free(pjsonBuffer);
	}
}

/// <summary>
///     Handle button timer event: if the button is pressed, report the event to the IoT Hub.
/// </summary>
static void ClickBoardChangePeriodEventHandler(EventData* eventData)
{
	if (ConsumeTimerFdEvent(singleTimerFd) != 0) {
		terminationRequired = true;
		return;
	}

	oled_state = 0;
	singleTimerFd = -1;
	close(singleTimerFd);

}

// event handler data structures. Only the event handler field needs to be populated.
static EventData clickBoardChangEventData = { .eventHandler = &ClickBoardChangePeriodEventHandler };

void checkTwinUpdateOfClickBoard(JSON_Object* desiredProperties, char* name)
{
	if (name == "clickBoardRelay1" && singleTimerFd == -1)
	{
		bool desiredValue = (bool)json_object_get_boolean(desiredProperties, name);
		if (desiredValue != relay_1_is_on)
		{
			if (desiredValue)
			{
				set_relay_start_time();
			}
			oled_state = 3;
			struct timespec clickBoardChangePeriod = { 10, 0 };
			singleTimerFd =
				CreateSingleTimerFdAndAddToEpoll(epollFd, &clickBoardChangePeriod, &clickBoardChangEventData, EPOLLIN);
			if (singleTimerFd < 0) {
				return -1;
			}
		}
	}
}

///<summary>
///		Parses received desired property changes.
///</summary>
///<param name="desiredProperties">Address of desired properties JSON_Object</param>
void deviceTwinChangedHandler(JSON_Object* desiredProperties)
{
	int result = 0;

	// Pull the twin version out of the message.  We use this value when we echo the new setting back to IoT Connect.
	if (json_object_has_value(desiredProperties, "$version") != 0)
	{
		desiredVersion = (int)json_object_get_number(desiredProperties, "$version");
	}

	// !IOT_CENTRAL_APPLICATION		

	for (int i = 0; i < (sizeof(twinArray) / sizeof(twin_t)); i++) {

		if (json_object_has_value(desiredProperties, twinArray[i].twinKey) != 0)
		{
			checkTwinUpdateOfClickBoard(desiredProperties, twinArray[i].twinKey);

			switch (twinArray[i].twinType) {
			case TYPE_BOOL:
				*(bool*)twinArray[i].twinVar = (bool)json_object_get_boolean(desiredProperties, twinArray[i].twinKey);
				result = GPIO_SetValue(*twinArray[i].twinFd, twinArray[i].active_high ? (GPIO_Value) * (bool*)twinArray[i].twinVar : !(GPIO_Value) * (bool*)twinArray[i].twinVar);

				if (result != 0) {
					Log_Debug("Fd: %d\n", twinArray[i].twinFd);
					Log_Debug("FAILURE: Could not set GPIO_%d, %d output value %d: %s (%d).\n", twinArray[i].twinGPIO, twinArray[i].twinFd, (GPIO_Value) * (bool*)twinArray[i].twinVar, strerror(errno), errno);
					terminationRequired = true;
				}
				Log_Debug("Received device update. New %s is %s\n", twinArray[i].twinKey, *(bool*)twinArray[i].twinVar ? "true" : "false");
				checkAndUpdateDeviceTwin(twinArray[i].twinKey, twinArray[i].twinVar, TYPE_BOOL, true);
				break;
			case TYPE_FLOAT:
				*(float*)twinArray[i].twinVar = (float)json_object_get_number(desiredProperties, twinArray[i].twinKey);
				Log_Debug("Received device update. New %s is %0.2f\n", twinArray[i].twinKey, *(float*)twinArray[i].twinVar);
				checkAndUpdateDeviceTwin(twinArray[i].twinKey, twinArray[i].twinVar, TYPE_FLOAT, true);
				break;
			case TYPE_INT:
				*(int*)twinArray[i].twinVar = (int)json_object_get_number(desiredProperties, twinArray[i].twinKey);
				Log_Debug("Received device update. New %s is %d\n", twinArray[i].twinKey, *(int*)twinArray[i].twinVar);
				checkAndUpdateDeviceTwin(twinArray[i].twinKey, twinArray[i].twinVar, TYPE_INT, true);
				break;

			case TYPE_STRING:
				strcpy((char*)twinArray[i].twinVar, (char*)json_object_get_string(desiredProperties, twinArray[i].twinKey));
				Log_Debug("Received device update. New %s is %s\n", twinArray[i].twinKey, (char*)twinArray[i].twinVar);
				checkAndUpdateDeviceTwin(twinArray[i].twinKey, twinArray[i].twinVar, TYPE_STRING, true);
				break;
			}
		}
	}
}
