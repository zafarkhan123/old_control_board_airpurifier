/**
 * @file timeDriver.c
 *
 * @brief time driver source file
 *
 * @dir timeDriver
 * @brief time driver folder
 *
 * @author matfio
 * @date 2021.08.31
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "timeDriver.h"

#include <string.h>
#include "esp_timer.h"

#include "location/location.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define LOCAL_TIME_STRING_LEN (32U)

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

uint32_t TimeDriverGetUTCUnixTime(void)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);

    return (uint32_t)tv_now.tv_sec;
}

uint32_t TimeDriverGetLocalUnixTime(void)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);

    float offset = LocationGetUtcOffset();
    offset = offset * 60.0f * 60.0f;

    return ((uint32_t)tv_now.tv_sec + (uint32_t)offset);
}

struct tm* TimeDriverGetUTCTime(void)
{
    static struct tm * timeInfo;
    time_t rawTime;
    
    time(&rawTime);
    timeInfo = localtime(&rawTime);

    return timeInfo;
}

struct tm* TimeDriverGetLocalTime(void)
{
    static struct tm * timeInfo;
    time_t rawTime;
    
    time(&rawTime);

    float offset = LocationGetUtcOffset();
    offset = offset * 60.0f * 60.0f;
    rawTime += (uint32_t)offset;

    timeInfo = localtime(&rawTime);

    return timeInfo;
}

int64_t TimeDriverGetSystemTickMs(void)
{
    return (esp_timer_get_time() / 1000U);
}

bool TimeDriverHasTimeElapsed(int64_t startTime, uint32_t deltaMsTime)
{
    if (TimeDriverGetSystemTickMs() - startTime > deltaMsTime) {
        return true;
    }
    return false;
}

void TimeDriverSetEspTime(struct tm* newTime)
{
    time_t rtcUnix = mktime(newTime);

    struct timeval newUnix = {
        .tv_sec = rtcUnix,
        .tv_usec = 0,
    };

    settimeofday(&newUnix, NULL);
}

void TimeDriverSetEspTimeByUnixTime(uint32_t newTime)
{
    struct timeval newUnix = {
        .tv_sec = newTime,
        .tv_usec = 0,
    };

    settimeofday(&newUnix, NULL);
}

char* TimeDriverGetLocalTimeStr(void)
{
    static char localTimeStr[LOCAL_TIME_STRING_LEN];
    memset(&localTimeStr, 0, LOCAL_TIME_STRING_LEN);

    struct tm * timeInfo = TimeDriverGetLocalTime();

    strftime(localTimeStr, LOCAL_TIME_STRING_LEN ,"%F %X", timeInfo);

    return localTimeStr;
}