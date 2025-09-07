/**
 * @file scheduler.c
 *
 * @brief Scheduler middleware source file
 *
 * @dir scheduler
 * @brief Scheduler middleware folder
 *
 * @author matfio
 * @date 2021.09.08
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "setting.h"

#include "scheduler.h"

#include <esp_log.h>

#include <string.h>
#include <time.h> 

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "timeDriver/timeDriver.h"
#include "nvsDriver/nvsDriver.h"

#include "factorySettingsDriver/factorySettingsDriver.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define MUTEX_TIMEOUT_MS (5U * 1000U)
#define NVS_KAY_NAME ("Scheduler")

// Convert value unix time seconds to unix time hours
#define SHEDULER_HOURS_FROM_UNIX_TIME(H) ((H) / ((60U) * (60U)))

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static bool sSchedulerIsChange;
static SemaphoreHandle_t sSchedulerMutex;
static Scheduler_t sScheduler;

static const char *TAG = "scheduler";

static const char* sDaysOfWeekString[SCHEDULER_DAY_COUNT] = {
    [SCHEDULER_DAY_MONDAY] = "Monday",
    [SCHEDULER_DAY_TUESDAY] = "Tuesday",
    [SCHEDULER_DAY_WEDNESDAY] = "Wednesday",
    [SCHEDULER_DAY_THURSDAY] = "Thursday",
    [SCHEDULER_DAY_FRIDAY] = "Friday",
    [SCHEDULER_DAY_SATURDAY] = "Saturday",
    [SCHEDULER_DAY_SUNDAY] = "Sunday",
};

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool SchedulerInit(void)
{
    sSchedulerMutex = xSemaphoreCreateMutex();
    if (sSchedulerMutex == NULL) {
        return false;
    }
    SchedulerLoad();
    
    return true;
}

bool SchedulerLoad(void)
{
    if (xSemaphoreTake(sSchedulerMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        Scheduler_t loadScheduler = {};
        uint16_t loadDataLen = sizeof(Scheduler_t);

        bool nvsRes = NvsDriverLoad(NVS_KAY_NAME, &loadScheduler, &loadDataLen);
        ESP_LOGI(TAG, "load data len %d", loadDataLen);

        if(nvsRes == true){
            if(loadDataLen == sizeof(Scheduler_t)){
                ESP_LOGI(TAG, "load scheduler from nvs");
                memcpy(&sScheduler, &loadScheduler, sizeof(Scheduler_t));
            }else{
                ESP_LOGI(TAG, "read factory mismatch size");

                nvsRes = FactorySettingsGetScheduler(&loadScheduler);
                if(nvsRes == true){
                    memcpy(&sScheduler, &loadScheduler, sizeof(Scheduler_t));
                    NvsDriverSave(NVS_KAY_NAME, &sScheduler, sizeof(Scheduler_t));
                }
            }
        }
        else {
            ESP_LOGI(TAG, "read factory scheduler");

            nvsRes = FactorySettingsGetScheduler(&loadScheduler);
            if(nvsRes == true){
                memcpy(&sScheduler, &loadScheduler, sizeof(Scheduler_t));
                nvsRes = NvsDriverSave(NVS_KAY_NAME, &sScheduler, sizeof(Scheduler_t));
            }
        }
        
        xSemaphoreGive(sSchedulerMutex);

        return nvsRes;
    }
    return false;
}

bool SchedulerSave(void)
{
    if (xSemaphoreTake(sSchedulerMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        bool res = NvsDriverSave(NVS_KAY_NAME, &sScheduler, sizeof(Scheduler_t));

        xSemaphoreGive(sSchedulerMutex);
        return res;
    }

    return false;
}

bool SchedulerGetAll(Scheduler_t* scheduler)
{
    if (xSemaphoreTake(sSchedulerMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        memcpy(scheduler, &sScheduler, sizeof(Scheduler_t));

        xSemaphoreGive(sSchedulerMutex);
        return true;
    }
    
    return false;
}

bool SchedulerSetAll(Scheduler_t* scheduler)
{
    if (xSemaphoreTake(sSchedulerMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        memcpy(&sScheduler, scheduler, sizeof(Scheduler_t));
        sSchedulerIsChange = true;

        xSemaphoreGive(sSchedulerMutex);
        return true;
    }
    
    return false;
}

bool SchedulerGetSingleDay(SchedulerDay_t day, SchedulerofDay_t* dayScheduler)
{
    if(day >= SCHEDULER_DAY_COUNT){
        return false;
    }

    if (xSemaphoreTake(sSchedulerMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        memcpy(dayScheduler, &sScheduler.days[day], sizeof(SchedulerofDay_t));

        xSemaphoreGive(sSchedulerMutex);
        return true;
    }
    
    return false;
}

bool SchedulerSetSingleDay(SchedulerDay_t day, SchedulerofDay_t* dayScheduler)
{
    if(day >= SCHEDULER_DAY_COUNT){
        return false;
    }

    if (xSemaphoreTake(sSchedulerMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        memcpy(&sScheduler.days[day], dayScheduler, sizeof(SchedulerofDay_t));
        sSchedulerIsChange = true;

        xSemaphoreGive(sSchedulerMutex);
        return true;
    }
    
    return false;
}

bool SchedulerGetSingleHourOfDay(SchedulerDay_t day, SchedulerHour_t hour, SettingDeviceStatust_t* hourScheduler)
{
    if(day >= SCHEDULER_DAY_COUNT){
        return false;
    }

    if(hour >= SCHEDULER_HOUR_COUNT){
        return false;
    }

    if (xSemaphoreTake(sSchedulerMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        memcpy(hourScheduler, &sScheduler.days[day].hours[hour], sizeof(SettingDeviceStatust_t));

        xSemaphoreGive(sSchedulerMutex);
        return true;
    }
    
    return false;
}

bool SchedulerGetCurrentDeviceStatus(SettingDevice_t* deviceSetting)
{
    struct tm * timeInfo = TimeDriverGetLocalTime();
    SettingDeviceStatust_t deviceStatusFromScheduler = {};

    // tm_wday -> days since Sunday – [0, 6]
    SchedulerDay_t day = SCHEDULER_DAY_MONDAY;
    if(timeInfo->tm_wday == 0){
        day = SCHEDULER_DAY_SUNDAY;
    }else{
        day = timeInfo->tm_wday - 1;
    }

    bool res = SchedulerGetSingleHourOfDay(day, timeInfo->tm_hour, &deviceStatusFromScheduler);
    if(res == false){
        return false;
    }

    memcpy(&deviceSetting->restore.deviceStatus, &deviceStatusFromScheduler, sizeof(SettingDeviceStatust_t));
    
    ESP_LOGI(TAG, "Is on %d, fan level %d, eco %d", deviceStatusFromScheduler.isDeviceOn, deviceStatusFromScheduler.fanLevel + 1, deviceStatusFromScheduler.isEkoOn);

    return true;
}

bool SchedulerIsDeviceStatusUpdateNeeded(SettingDevice_t* deviceSetting)
{
    static bool sRunFirstTime = true;
    static uint32_t sLastHour;

    // only if device mode is automatical scheduler is used
    if(deviceSetting->restore.deviceMode == DEVICE_MODE_MANUAL){
        return false;
    }

    // when ther is an error ther is not scheduler support
    if(deviceSetting->alarmError.isDetected == true){
        return false;
    }

    uint32_t unixTime = TimeDriverGetLocalUnixTime();   

    // scheduler has been changed
    if(sSchedulerIsChange == true)
    {
        if (xSemaphoreTake(sSchedulerMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
            sSchedulerIsChange = false;
            xSemaphoreGive(sSchedulerMutex);

            sLastHour = SHEDULER_HOURS_FROM_UNIX_TIME(unixTime);

            ESP_LOGI(TAG, "Time after update %s", TimeDriverGetLocalTimeStr());
        }

        return true;
    }
    
    // after restart, we need to get the current scheduler device plan
    if(sRunFirstTime == true){
        sRunFirstTime = false;
        sLastHour = SHEDULER_HOURS_FROM_UNIX_TIME(unixTime);
        ESP_LOGI(TAG, "Time after restart %s", TimeDriverGetLocalTimeStr());

        return true;
    }

    // a full hour has passed
    if((SHEDULER_HOURS_FROM_UNIX_TIME(unixTime)) != sLastHour){
        sLastHour = SHEDULER_HOURS_FROM_UNIX_TIME(unixTime);
        
        ESP_LOGI(TAG, "Time %s", TimeDriverGetLocalTimeStr());

        return true;
    }

    return false;
}

const char* SchedulerGetStringDayName(SchedulerDay_t day)
{
    if(day >= SCHEDULER_DAY_COUNT){
        return NULL;
    }

    return sDaysOfWeekString[day];
}

void SchedulerPrintf(Scheduler_t* scheduler)
{
    ESP_LOGI(TAG, "Scheduler size %d", sizeof(Scheduler_t));
    for(uint16_t dayIdx = 0; dayIdx < SCHEDULER_DAY_COUNT; ++dayIdx)
    {
        for(uint16_t hourIdx = 0; hourIdx < SCHEDULER_HOUR_COUNT; ++hourIdx)
        {
            if(scheduler->days[dayIdx].hours[hourIdx].isDeviceOn == true)
            {
                ESP_LOGI(TAG, "Day %d-> H %d [On, F %d, eco %d]", (dayIdx + 1), (hourIdx), 
                (scheduler->days[dayIdx].hours[hourIdx].fanLevel + 1),
                scheduler->days[dayIdx].hours[hourIdx].isEkoOn);
            }
            else{
                ESP_LOGI(TAG, "Day %d-> H %d [Off, F %d, eco %d]", (dayIdx + 1), (hourIdx), 
                (scheduler->days[dayIdx].hours[hourIdx].fanLevel),
                scheduler->days[dayIdx].hours[hourIdx].isEkoOn);
            }
        }
    }
}