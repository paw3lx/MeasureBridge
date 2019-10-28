#include "time_helper.h"

#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <applibs/log.h>
#include <stdio.h> 

extern volatile sig_atomic_t terminationRequired;
char currentTimeBuffer[26];
char elapsedTimeBuffer[10];

struct timespec relayStartTime;

void timespec_diff(const struct timespec* start, const struct timespec* stop, struct timespec* result)
{
	if ((stop->tv_nsec - start->tv_nsec) < 0) {
		result->tv_sec = stop->tv_sec - start->tv_sec - 1;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
	}
	else {
		result->tv_sec = stop->tv_sec - start->tv_sec;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}

	return;
}

void print_time(struct timespec _time)
{
	char _currentTimeBuffer[26];
	if (!asctime_r((localtime(&_time.tv_sec)), (char* restrict) & _currentTimeBuffer)) {
		Log_Debug("ERROR: asctime_r failed with error code: %s (%d).\n", strerror(errno),
			errno);
		terminationRequired = true;
		return;
	}

	// Remove the new line at the end of 'displayTimeBuffer'
	_currentTimeBuffer[strlen(_currentTimeBuffer) - 1] = '\0';
	size_t tznameIndex = ((localtime(&_time.tv_sec))->tm_isdst) ? 1 : 0;
	Log_Debug("Time:     %s %s\n", _currentTimeBuffer, tzname[tznameIndex]);
}
void update_current_time(void)
{
	struct timespec _currentTime;
	if (clock_gettime(CLOCK_REALTIME, &_currentTime) == -1) {
		Log_Debug("ERROR: clock_gettime failed with error code: %s (%d).\n", strerror(errno),
			errno);
		terminationRequired = true;
		return;
	}
	else {
		if (!asctime_r((localtime(&_currentTime.tv_sec)), (char* restrict) & currentTimeBuffer)) {
			Log_Debug("ERROR: asctime_r failed with error code: %s (%d).\n", strerror(errno),
				errno);
			terminationRequired = true;
			return;
		}

		// Remove the new line at the end of 'displayTimeBuffer'
		currentTimeBuffer[strlen(currentTimeBuffer) - 1] = '\0';
		size_t tznameIndex = ((localtime(&_currentTime.tv_sec))->tm_isdst) ? 1 : 0;
		Log_Debug("Local time:     %s %s\n", currentTimeBuffer, tzname[tznameIndex]);
	}
}

void set_relay_start_time(void)
{
	struct timespec _currentTime;
	if (clock_gettime(CLOCK_REALTIME, &_currentTime) == -1) {
		Log_Debug("ERROR: clock_gettime failed with error code: %s (%d).\n", strerror(errno),
			errno);
		terminationRequired = true;
		return;
	}
	relayStartTime.tv_nsec = _currentTime.tv_nsec;
	relayStartTime.tv_sec = _currentTime.tv_sec;
	Log_Debug("Setting relay start time\n");
}

void update_relay_elapsed_time(void)
{
	struct timespec _currentTime;
	if (clock_gettime(CLOCK_REALTIME, &_currentTime) == -1) {
		Log_Debug("ERROR: clock_gettime failed with error code: %s (%d).\n", strerror(errno),
			errno);
		terminationRequired = true;
		return;
	}
	else {
		struct timespec _diff;
		timespec_diff(&relayStartTime, &_currentTime, &_diff);

		struct tm* ptm = localtime(&_diff);

		if (ptm == NULL) {

			return;
		}
		strftime(elapsedTimeBuffer, 10, "%H:%M:%S", ptm);
		//print_time(relayStartTime);
		Log_Debug("Diff: %s\n", elapsedTimeBuffer);
	}
}

void update_time_view(void)
{
	update_current_time();
	update_relay_elapsed_time();
}