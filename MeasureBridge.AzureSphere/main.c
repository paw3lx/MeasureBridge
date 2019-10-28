#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"
#include "mt3620.h"
#include "spi.h"
#include "ac_current_click.h"
#include "i2c.h"
#include "deviceTwin.h"
#include "azure_iot_utilities.h"
#include "connection_strings.h"

#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/spi.h>
#include <applibs/wificonfig.h>
#include <azureiot/iothub_device_client_ll.h>

// Provide local access to variables in other files
extern twin_t twinArray[];
extern int twinArraySize;
extern IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle;

static int InitPeripheralsAndHandlers(void);
static void TerminationHandler(int signalNumber);
static void ClosePeripheralsAndHandlers(void);

int epollFd = -1;

// Termination state
volatile sig_atomic_t terminationRequired = false;
bool versionStringSent = false;

int clickSocket1Relay1Fd = -1;
int clickSocket1Relay2Fd = -1;

bool clkBoardRelay1IsOn = false;
bool clkBoardRelay2IsOn = false;

static int buttonPollTimerFd = -1;
static int buttonBGpioFd = -1;

static GPIO_Value_Type buttonBState = GPIO_Value_High;

int main(void)
{
	Log_Debug("MeasureBridge application starting.\n");
	if (InitPeripheralsAndHandlers() != 0) {
		terminationRequired = true;
	}

	// Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
	while (!terminationRequired) {
		if (WaitForEventAndCallHandler(epollFd) != 0) {
			terminationRequired = true;
		}
		if (!AzureIoT_SetupClient()) {
			Log_Debug("ERROR: Failed to set up IoT Hub client\n");
			break;
		}

		WifiConfig_ConnectedNetwork network;
		int result = WifiConfig_GetCurrentNetwork(&network);

		if (result < 0) {
			Log_Debug("INFO: Not currently connected to a WiFi network.\n");
		}
		else {
			AzureIoT_DoPeriodicTasks();
		}
	}

	ClosePeripheralsAndHandlers();
	Log_Debug("Application exiting.\n");
}

/// <summary>
///     Handle button timer event: if the button is pressed, report the event to the IoT Hub.
/// </summary>
static void ButtonTimerEventHandler(EventData* eventData)
{

	bool sendTelemetryButtonA = false;
	bool sendTelemetryButtonB = false;

	if (ConsumeTimerFdEvent(buttonPollTimerFd) != 0) {
		terminationRequired = true;
		return;
	}


	// Check for button B press
	GPIO_Value_Type newButtonBState;
	int result = GPIO_GetValue(buttonBGpioFd, &newButtonBState);
	if (result != 0) {
		Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
		terminationRequired = true;
		return;
	}

	// If the B button has just been pressed/released, send a telemetry message
	// The button has GPIO_Value_Low when pressed and GPIO_Value_High when released
	if (newButtonBState != buttonBState) {
		if (newButtonBState == GPIO_Value_Low) {
			// Send Telemetry here
			Log_Debug("Button B pressed!\n");
			sendTelemetryButtonB = true;

			//// OLED

			oled_state++;

			if (oled_state > 1)
			{
				oled_state = 0;
			}
		}
		else {
			Log_Debug("Button B released!\n");

		}

		// Update the static variable to use next time we enter this routine
		buttonBState = newButtonBState;


	}

}

// event handler data structures. Only the event handler field needs to be populated.
static EventData buttonEventData = { .eventHandler = &ButtonTimerEventHandler };

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int InitPeripheralsAndHandlers(void)
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = TerminationHandler;
	sigaction(SIGTERM, &action, NULL);

	epollFd = CreateEpollFd();
	if (epollFd < 0) {
		return -1;
	}

	if (initI2c() == -1) {
		return -1;
	}

	if (InitSpi() == -1) {
		return -1;
	}

	// Traverse the twin Array and for each GPIO item in the list open the file descriptor
	for (int i = 0; i < twinArraySize; i++) {

		// Verify that this entry is a GPIO entry
		if (twinArray[i].twinGPIO != NO_GPIO_ASSOCIATED_WITH_TWIN) {

			*twinArray[i].twinFd = -1;

			// For each item in the data structure, initialize the file descriptor and open the GPIO for output.  Initilize each GPIO to its specific inactive state.
			*twinArray[i].twinFd = (int)GPIO_OpenAsOutput(twinArray[i].twinGPIO, GPIO_OutputMode_PushPull, twinArray[i].active_high ? GPIO_Value_Low : GPIO_Value_High);

			if (*twinArray[i].twinFd < 0) {
				Log_Debug("ERROR: Could not open LED %d: %s (%d).\n", twinArray[i].twinGPIO, strerror(errno), errno);
				return -1;
			}
		}
	}

	// Open button B GPIO as input
	Log_Debug("Opening Starter Kit Button B as input.\n");
	buttonBGpioFd = GPIO_OpenAsInput(MT3620_GPIO13); //MT3620_GPIO13 MT3620_GPIO2
	if (buttonBGpioFd < 0) {
		Log_Debug("ERROR: Could not open button B GPIO: %s (%d).\n", strerror(errno), errno);
		return -1;
	}

	// Set up a timer to poll the buttons

	struct timespec buttonPressCheckPeriod = { 0, 1000000 };
	buttonPollTimerFd =
		CreateTimerFdAndAddToEpoll(epollFd, &buttonPressCheckPeriod, &buttonEventData, EPOLLIN);
	if (buttonPollTimerFd < 0) {
		return -1;
	}

	AzureIoT_SetDeviceTwinUpdateCallback(&deviceTwinChangedHandler);

	return 0;
}

static void TerminationHandler(int signalNumber)
{
	terminationRequired = true;
}

static void ClosePeripheralsAndHandlers(void)
{
	Log_Debug("Closing file descriptors.\n");
	closeSpi();
	closeI2c();
}
