#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/gpio.h>

int main(void)
{
    const struct timespec sleepTime = {1, 0};
    while (true) {
        nanosleep(&sleepTime, NULL);
    }
}