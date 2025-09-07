/**
 * @file timerDriver.c
 *
 * @brief Timer driver source file
 *
 * @dir timerDriver
 * @brief Timer Driver driver folder
 *
 * @author matfio
 * @date 2021.09.02
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "config.h"

#include <esp_log.h>

#include "driver/timer.h"

#include "timerDriver.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define TIMER_DIVIDER (8U * 1000U)

#if CFG_TEST_TYPE == CFG_TEST_QUICK_TIMER
    #define TIMER_DIVIDER (2U) 
    #warning "TEST MODE Internal timers will run faster X 4000"
#endif

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

typedef struct
{
    timer_group_t timerGroup;
    timer_idx_t timerIdx;
}__attribute__ ((packed)) TimerSetting_t;

const static TimerSetting_t sTimerSetting[TIMER_NAME_COUNTER] = {
    [TIMER_NAME_HEPA] =      {.timerGroup = TIMER_GROUP_0, .timerIdx = TIMER_0},
    [TIMER_NAME_GLOBAL_ON] = {.timerGroup = TIMER_GROUP_0, .timerIdx = TIMER_1},
    [TIMER_NAME_UV_LAMP_1] = {.timerGroup = TIMER_GROUP_1, .timerIdx = TIMER_0},
    [TIMER_NAME_UV_LAMP_2] = {.timerGroup = TIMER_GROUP_1, .timerIdx = TIMER_1},
};

static const char *TAG = "timerD";

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool TimerDriverInit(void)
{
    for (uint16_t idx = 0; idx < TIMER_NAME_COUNTER; ++idx)
    {
        timer_config_t config = {
            .divider = TIMER_DIVIDER,
            .counter_dir = TIMER_COUNT_UP,
            .counter_en = TIMER_PAUSE,
            .alarm_en = TIMER_ALARM_DIS,
            .auto_reload = false,
        }; // default clock source is APB
        timer_init(sTimerSetting[idx].timerGroup, sTimerSetting[idx].timerIdx, &config);
    }
    
    return true;
}

bool TimerDriverSetTimers(const SettingDevice_t* setting)
{
    for (uint16_t idx = 0; idx < TIMER_NAME_COUNTER; ++idx)
    {
        timer_set_counter_value(sTimerSetting[idx].timerGroup, sTimerSetting[idx].timerIdx, setting->restore.liveTime[idx]);
    }
    return true;
}

bool TimerDriverGetCounter(SettingTimerName_t timer, uint64_t* counter)
{
    if(timer >=  TIMER_NAME_COUNTER){
        return false;
    }

    timer_get_counter_value(sTimerSetting[timer].timerGroup, sTimerSetting[timer].timerIdx, counter);

    return true;
}

void TimerDriverClearCounter(SettingTimerName_t timer)
{
     if(timer >=  TIMER_NAME_COUNTER){
        return;
    }

    SettingDevice_t setting = {};

    SettingGet(&setting);
    setting.restore.liveTime[timer] = 0;
    SettingSet(&setting);

    timer_set_counter_value(sTimerSetting[timer].timerGroup, sTimerSetting[timer].timerIdx, setting.restore.liveTime[timer]);
}

bool TimerDriverGetCounterSec(SettingTimerName_t timer, uint64_t* counter)
{
    if(TimerDriverGetCounter(timer, counter) == false){
        return false;
    }

    *counter = (*counter / (1000U * 10U));

    return true;
}

bool TimerDriverPause(SettingTimerName_t timer)
{
    if(timer >=  TIMER_NAME_COUNTER){
        return false;
    }
    timer_pause(sTimerSetting[timer].timerGroup, sTimerSetting[timer].timerIdx);

    return true;
}

bool TimerDriverStart(SettingTimerName_t timer)
{
    if(timer >=  TIMER_NAME_COUNTER){
        return false;
    }
    timer_start(sTimerSetting[timer].timerGroup, sTimerSetting[timer].timerIdx);

    return true;
}

bool TimerDriverUpdateTimerSetting(SettingDevice_t* setting)
{
    uint64_t counter = 0;

    for (uint16_t idx = 0; idx < TIMER_NAME_COUNTER; ++idx){
        TimerDriverGetCounter(idx, &counter);

        setting->restore.liveTime[idx] = counter;
        ESP_LOGI(TAG, "%u -> %lld [S]", idx, TIMER_DRIVER_RAW_DATA_TO_SECOND(counter));
    }
    return true;
}