/**
 * @file fan.c
 *
 * @brief Fan middleware source file
 *
 * @dir fanDriver
 * @brief Fan middleware folder
 *
 * @author matfio
 * @date 2021.08.27
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "config.h"
#include "setting.h"

#include <string.h>

#include "esp_log.h"

#include "fanDriver/fanDriver.h"
#include "factorySettingsDriver/factorySettingsDriver.h"
#include "timeDriver/timeDriver.h"

#include "fan/fan.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define FAN_DAFAULT_OFF_DUTY (0)
#define FAN_DAFAULT_UV_LAMP_BALLAST_RELAY (FAN_LEVEL_1)

#define MUTEX_TIMEOUT_MS (1U * 1000U)

#define TIMER_PERIOD_MS (1U * 1000U)

#define TIME_NEED_TO_START_FAN_SEC (30U * 1000U)

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static bool sFanIsOff = true;

static int16_t sTachoRevolutionsPerSecond;

// PWM counter timer size 12 bits
static uint32_t sFanLevelPwmDuty[FAN_LEVEL_COUNT];

static int64_t sFanStartTime = 0;

static const char* TAG = "fan";
static const char* sTimerName = "fanTimer";

static TimerHandle_t sTimerHandler;
static SemaphoreHandle_t sFanTimerMutex;

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Calback function. Call when the timer expires
 *  @param pxTimer pointer to TimerHandle_t
 */
static void vFanTimerCallback(TimerHandle_t pxTimer);

/** @brief Free Rtos timer init and start
 *  @return return true if success
 */
static bool InitTimer(void);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool FanInit(void)
{
    bool res = true;
    
    res &= FanDriverInit();
    res &= FanDriverSetDuty(FAN_DAFAULT_OFF_DUTY);

    ESP_LOGI(TAG, "read factory pwm values");
    uint32_t pwmValue = 0;

    for(uint16_t idx = 0; idx < FAN_LEVEL_COUNT; ++idx){
        res &= FactorySettingsGetPwmFanLevel(idx, &pwmValue);

        sFanLevelPwmDuty[idx] = pwmValue;
    }

    res &= InitTimer();

    return res;
}

bool FanLevelChange(const SettingDevice_t* settingDevice)
{
    static SettingFanLevel_t sLastFanLevel;

    bool res = true;
    if(settingDevice->alarmError.isDetected == true)
    {
        if((settingDevice->alarmError.stuckRelayUvLamp1 == true) || (settingDevice->alarmError.stuckRelayUvLamp2 == true)){
            uint32_t lampRelayFanLevel = FanGetActualPwmFanLevel(FAN_DAFAULT_UV_LAMP_BALLAST_RELAY);
            ESP_LOGI(TAG, "fan emergency set level 1");
            res &= FanDriverSetDuty(lampRelayFanLevel);

            return res;
        }

        ESP_LOGI(TAG, "fan emergency off");
        res &= FanDriverSetDuty(FAN_DAFAULT_OFF_DUTY);

        return res;
    }

    if(settingDevice->restore.deviceStatus.isDeviceOn == false)
    {
        res &= FanDriverSetDuty(FAN_DAFAULT_OFF_DUTY);
        sFanIsOff = true;

        ESP_LOGI(TAG, "fan off");
        
        return res;
    }

    if((sFanIsOff == false) && (sLastFanLevel == settingDevice->restore.deviceStatus.fanLevel)){
        return true;
    }
    sFanIsOff = false;
    sLastFanLevel = settingDevice->restore.deviceStatus.fanLevel;

    res &= FanDriverSetDuty(sFanLevelPwmDuty[sLastFanLevel]);
      
    sFanStartTime = TimeDriverGetSystemTickMs();
    ESP_LOGI(TAG, "%llu change fan level to %d", sFanStartTime, (sLastFanLevel + 1));

    return res;
}

uint32_t FanGetActualPwmFanLevel(SettingFanLevel_t level)
{
    if(level >= FAN_LEVEL_COUNT){
        return 0;
    }

    return sFanLevelPwmDuty[level];
}

void FanSetNewPwmFanLevel(SettingFanLevel_t level, uint32_t newPwmValue)
{
    if(level >= FAN_LEVEL_COUNT){
        return ;
    }

    sFanLevelPwmDuty[level] = newPwmValue;
}

FanTachoState_t FanGetTachoRevolutionsPerSecond(int16_t* revolutions)
{
    if (xSemaphoreTake(sFanTimerMutex, MUTEX_TIMEOUT_MS) == pdTRUE)
    {
        *revolutions = sTachoRevolutionsPerSecond;
         xSemaphoreGive(sFanTimerMutex);

        if(sFanIsOff == true){
            return FAN_TACHO_DEVICE_OFF;
        }

        if(TimeDriverHasTimeElapsed(sFanStartTime, TIME_NEED_TO_START_FAN_SEC) == false){
            return FAN_TACHO_STARTS;
        }

        return FAN_TACHO_WORKS;
    }

    return FAN_TACHO_ERROR;
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static void vFanTimerCallback(TimerHandle_t pxTimer)
{
    if (xSemaphoreTake(sFanTimerMutex, MUTEX_TIMEOUT_MS) == pdTRUE)
    {
        sTachoRevolutionsPerSecond = FanDriverGetTachoCount();
        
        xSemaphoreGive(sFanTimerMutex);
    }
}

static bool InitTimer(void)
{
    sFanTimerMutex = xSemaphoreCreateMutex();
    if (sFanTimerMutex == NULL) {
        ESP_LOGE(TAG, "Mutex Fail");
        return false;
    }

    sTimerHandler = xTimerCreate(sTimerName, TIMER_PERIOD_MS, pdTRUE, (void *) 0, vFanTimerCallback);
    if(sTimerHandler == NULL){
        ESP_LOGE(TAG, "Can't allocate heap");
        return false;
    }

    if(xTimerStart(sTimerHandler, 0) != pdPASS){
        ESP_LOGE(TAG, "Start Timer Fail");
        return false;
    }

    return true;
}