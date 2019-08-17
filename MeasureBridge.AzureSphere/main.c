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

#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/spi.h>

static int InitPeripheralsAndHandlers(void);
static void TerminationHandler(int signalNumber);
static void ClosePeripheralsAndHandlers(void);

static int epollFd = -1;

// Termination state
static volatile sig_atomic_t terminationRequired = false;

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
