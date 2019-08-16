#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"
#include "mt3620.h"
#include "ac_current_click.h"
#include "i2c.h"

#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/spi.h>

static int InitPeripheralsAndHandlers(void);
static void TerminationHandler(int signalNumber);
static void ClosePeripheralsAndHandlers(void);

static int epollFd = -1;
static int spiFd = -1;

// Termination state
static volatile sig_atomic_t terminationRequired = false;

int main(void)
{
	Log_Debug("MeasureBridge application starting.\n");
	if (InitPeripheralsAndHandlers() != 0) {
		terminationRequired = true;
	}

	if (initI2c() == -1) {
		return -1;
	}

	// Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
	while (!terminationRequired) {
		if (WaitForEventAndCallHandler(epollFd) != 0) {
			terminationRequired = true;
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

	SPIMaster_Config config;
	int ret = SPIMaster_InitConfig(&config);
	if (ret != 0) {
		Log_Debug("ERROR: SPIMaster_InitConfig = %d errno = %s (%d)\n", ret, strerror(errno),
			errno);
		return -1;
	}
	config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;
	spiFd = SPIMaster_Open(MT3620_ISU1_SPI, MT3620_SPI_CS_A, &config);
	if (spiFd < 0) {
		Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	int result = SPIMaster_SetBusSpeed(spiFd, 976000);
	if (result != 0) {
		Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	result = SPIMaster_SetMode(spiFd, SPI_Mode_0);
	if (result != 0) {
		Log_Debug("ERROR: SPIMaster_SetMode: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

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
