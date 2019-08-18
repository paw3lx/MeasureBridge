#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"
#include "spi.h"
#include "mt3620.h"
#include "ac_current_click.h"

#include <applibs/log.h>
#include <applibs/spi.h>

int spiFd = -1;
extern int epollFd;
static int acCurrentFd = -1;
extern volatile sig_atomic_t terminationRequired;

void AcCurrentEventHandler(EventData* eventData)
{
	// Consume the event.  If we don't do this we'll come right back 
	// to process the same event again
	if (ConsumeTimerFdEvent(acCurrentFd) != 0) {
		terminationRequired = true;
		return;
	}
	float ac = GetCurrentAC(3);
	ac_current = ac;
	Log_Debug("Current AC = %f \n", ac);
	UpdateACCurrent();
}

int InitSpi(void)
{
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

	struct timespec readDataeriod = { .tv_sec = 3.0,.tv_nsec = 0 };
	// event handler data structures. Only the event handler field needs to be populated.
	static EventData accelEventData = { .eventHandler = &AcCurrentEventHandler };
	acCurrentFd = CreateTimerFdAndAddToEpoll(epollFd, &readDataeriod, &accelEventData, EPOLLIN);
	if (acCurrentFd < 0) {
		return -1;
	}

	return 0;
}

void closeSpi(void) {

	CloseFdAndPrintError(spiFd, "Spi");
	CloseFdAndPrintError(acCurrentFd, "accelTimer");
}

