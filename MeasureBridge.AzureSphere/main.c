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

static int epollFd = -1;

// Termination state
volatile sig_atomic_t terminationRequired = false;
bool versionStringSent = false;

int clickSocket1Relay1Fd = -1;
int clickSocket1Relay2Fd = -1;

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
		// Setup the IoT Hub client.
		// Notes:
		// - it is safe to call this function even if the client has already been set up, as in
		//   this case it would have no effect;
		// - a failure to setup the client is a fatal error.
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
	CloseFdAndPrintError(spiFd, "Spi");
}
