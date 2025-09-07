/**
 * @file uvLamp.c
 *
 * @brief UV Lamp middleware source file
 *
 * @dir uvLamp
 * @brief uvLamp middleware folder
 *
 * @author matfio
 * @date 2021.11.02
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "config.h"
#include "setting.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#include "esp_log.h"

#include "timerDriver/timerDriver.h"
#include "adcDriver/adcDriver.h"
#include "factorySettingsDriver/factorySettingsDriver.h"
#include "timeDriver/timeDriver.h"

#include "utils/meanFilter/meanFilter.h"

#include "uvLamp.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define TIMER_PERIOD_MS (1U * 1000U)
#define MUTEX_TIMEOUT_MS (1U * 1000U)

#define TIME_NEEDED_TO_STABILIZE_MEASUREMENT (90U * 1000U)

#define LAMP_ON_DELAY_TIMIE_MS (CFG_UV_LAMP_ON_DELAY_TIMIE_SEC * 1000U)
#define UV_LAMP_ECO_MODE_SWITCH_TIMIE_MS (CFG_UV_LAMP_ECO_MODE_SWITCH_TIMIE_SEC * 1000U)      // 30 minute

#if CFG_TEST_TYPE == CFG_TEST_ECO
    #warning "TEST MODE eco test. Fast eco mode switch every 2 minutes"
    #define UV_LAMP_ECO_MODE_SWITCH_TIMIE_MS ((2U * 60U) * 1000U)                             // 2 minute
#endif

#define MEAN_BUFFER_SIZE (32U)

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static uint32_t sOffMinInMiliVolt;
static uint32_t sOffMaxInMiliVolt;

static uint32_t sOnMinInMiliVolt;
static uint32_t sOnMaxInMiliVolt;

static const char* TAG = "uvLamp";
static const char* sTimerName = "uvLampTimer";

static TimerHandle_t sTimerHandler;
static SemaphoreHandle_t sUvLampTimerMutex;

static meanFilter_t sFilterUvLamp1;
static meanFilter_t sFilterUvLamp2;

static uint32_t sMeanVoltageUvLamp1;
static uint32_t sMeanVoltageUvLamp2;

static int64_t sUvLampBallastStabilizationTime;
static int64_t sUvLampDelayTime;
static int64_t sUvLampNextEcoModeSwitchTime;

/******************************************************************************
                        PRIVATE FUNCTION DECLARATION
******************************************************************************/

/** @brief Get lamp number witch is more worn out
 *  @return lamp number
 */
static uvLampNumber_t WhichLampWornOutMost(void);

/** @brief Calback function. Call when the timer expires
 *  @param pxTimer pointer to TimerHandle_t
 */
static void vUvLampTimerCallback(TimerHandle_t pxTimer);

/** @brief Free Rtos timer init and start
 *  @return return true if success
 */
static bool InitTimer(void);

/** @brief Calculate min, max for uv lamp on off depending on the voltage
 *  @return return true if success
 */
static bool SetMinMaxVoltageLevels(void);

/** @brief Initialize mean measure structure
 *  @return return true if success
 */
static bool InitMeanStruct(void);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool UvLampInit(void)
{
    bool res = true;

    res &= UvLampDriverInit();
    res &= SetMinMaxVoltageLevels();
    res &= InitMeanStruct();
    res &= InitTimer();

    return res;
}

bool UvLampManagement(SettingDevice_t* setting)
{
    bool res = true;
    static bool sEcoModeRunFirstTime = true;

    if(setting->alarmError.isDetected == true){        
        setting->uvLamp1On = false;
        setting->uvLamp2On = false;
        
        sEcoModeRunFirstTime = true;

        ESP_LOGI(TAG, "Turn off uv lamps");
        UvLampEmergencyOff();

        return res;
    }

    if(setting->restore.deviceStatus.isDeviceOn == false){
        // Turn off all timers and on 2 uv lamps        
        setting->uvLamp1On = false;
        setting->uvLamp2On = false;

        sEcoModeRunFirstTime = true;
        return res;
    }

    if(setting->restore.deviceStatus.isEkoOn == false){
        // Eco mode off
        // Turn on all timers and on 2 uv lamps
        setting->uvLamp1On = true;
        setting->uvLamp2On = true;  

        sEcoModeRunFirstTime = true;
    }else{
        // Eco mode on
        if((sEcoModeRunFirstTime == false) && (TimeDriverHasTimeElapsed(sUvLampNextEcoModeSwitchTime, UV_LAMP_ECO_MODE_SWITCH_TIMIE_MS) == false)){
            return res;
        }

        sEcoModeRunFirstTime = false;
        sUvLampNextEcoModeSwitchTime = TimeDriverGetSystemTickMs();

        if(WhichLampWornOutMost() == UV_LAMP_1){
            setting->uvLamp1On = false;
            setting->uvLamp2On = true;
        }
        else{
            setting->uvLamp1On = true;
            setting->uvLamp2On = false;
        }
    }

    return res;
}

bool UvLampExecute(SettingDevice_t* setting)
{
    static bool sUvLampDelayRun;
    static bool sUvLamp1IsOnNow;
    static bool sUvLamp2IsOnNow;

    bool res = true;

    if((sUvLampDelayRun == false) && ((sUvLamp1IsOnNow != setting->uvLamp1On) || (sUvLamp2IsOnNow != setting->uvLamp2On)))
    {
        sUvLampDelayRun = true;
        
        sUvLamp1IsOnNow = setting->uvLamp1On;
        sUvLamp2IsOnNow = setting->uvLamp2On;

        sUvLampDelayTime = TimeDriverGetSystemTickMs();

        res &= UvLampDriverSetLevel(UV_LAMP_1, sUvLamp1IsOnNow);
        if(sUvLamp1IsOnNow == true){
            res &= TimerDriverStart(TIMER_NAME_UV_LAMP_1);
        }else{
            res &= TimerDriverPause(TIMER_NAME_UV_LAMP_1);
        }

        sUvLampBallastStabilizationTime = TimeDriverGetSystemTickMs();
        
        if((sUvLamp1IsOnNow == false) && (sUvLamp2IsOnNow == false)){
            res &= TimerDriverPause(TIMER_NAME_HEPA);
            res &= TimerDriverPause(TIMER_NAME_GLOBAL_ON);

            res &= TimerDriverPause(TIMER_NAME_UV_LAMP_2);
            res &= UvLampDriverSetLevel(UV_LAMP_2, sUvLamp2IsOnNow);
            sUvLampDelayRun = false;
            // when device is off we dont need any delay
        }
        else{
            res &= TimerDriverStart(TIMER_NAME_HEPA);
            res &= TimerDriverStart(TIMER_NAME_GLOBAL_ON);
        }
    }

    if((sUvLampDelayRun == true) && (TimeDriverHasTimeElapsed(sUvLampDelayTime, LAMP_ON_DELAY_TIMIE_MS) == true)){
        ESP_LOGI(TAG, "delay end");
        res &= UvLampDriverSetLevel(UV_LAMP_2, sUvLamp2IsOnNow);
        if(sUvLamp2IsOnNow == true){
            res &= TimerDriverStart(TIMER_NAME_UV_LAMP_2);
        }else{
            res &= TimerDriverPause(TIMER_NAME_UV_LAMP_2);
        }

        sUvLampDelayRun = false;

        sUvLampBallastStabilizationTime = TimeDriverGetSystemTickMs();
    }

    return res;
}

uvLampStatus_t UvLampDriverGetUvLampStatus(uvLampNumber_t lampNumber)
{   
    if(lampNumber >= UV_LAMP_COUNT){
        return UV_LAMP_STATUS_UNKNOWN;
    }

    if(TimeDriverHasTimeElapsed(sUvLampBallastStabilizationTime, TIME_NEEDED_TO_STABILIZE_MEASUREMENT) == false){
            return UV_LAMP_STATUS_UNKNOWN;
    }

    uint32_t miliVoltage = UvLampGetMeanMiliVolt(lampNumber);

    if((miliVoltage >= sOffMinInMiliVolt) && (miliVoltage <= sOffMaxInMiliVolt)){
        return UV_LAMP_STATUS_OFF;
    }

    if((miliVoltage >= sOnMinInMiliVolt) && (miliVoltage <= sOnMaxInMiliVolt)){
        return UV_LAMP_STATUS_ON;
    }
 
    return UV_LAMP_STATUS_ERROR;
}

void UvLampEmergencyOff(void)
{
    UvLampDriverSetLevel(UV_LAMP_1, false);
    UvLampDriverSetLevel(UV_LAMP_2, false);

    ESP_LOGI(TAG, "uv lamp emergency off");
}

uint32_t UvLampGetMeanMiliVolt(uvLampNumber_t lampNumber)
{
    uint32_t miliVoltage = 0;

    if(lampNumber >= UV_LAMP_COUNT){
        return miliVoltage;
    }

    if (xSemaphoreTake(sUvLampTimerMutex, MUTEX_TIMEOUT_MS) == pdTRUE)
    {
        if(lampNumber == UV_LAMP_1){
            miliVoltage = sMeanVoltageUvLamp1;
        }
        else if(lampNumber == UV_LAMP_2){
            miliVoltage = sMeanVoltageUvLamp2;
        }

        xSemaphoreGive(sUvLampTimerMutex);
    }

    return miliVoltage;
}
/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static uvLampNumber_t WhichLampWornOutMost(void)
{
    uint64_t uv1Counter = 0;
    uint64_t uv2Counter = 0;

    uvLampNumber_t lampNumber = UV_LAMP_1;

    TimerDriverGetCounterSec(TIMER_NAME_UV_LAMP_1, &uv1Counter);
    TimerDriverGetCounterSec(TIMER_NAME_UV_LAMP_2, &uv2Counter);

    if(uv1Counter < uv2Counter){

        lampNumber = UV_LAMP_2;
    }

    ESP_LOGI(TAG, "worn out more %d", (lampNumber + 1));
    ESP_LOGI(TAG, "1 -> %lld [S]", uv1Counter);
    ESP_LOGI(TAG, "2 -> %lld [S]", uv2Counter);

    return lampNumber;
}

static void vUvLampTimerCallback(TimerHandle_t pxTimer)
{
    if (xSemaphoreTake(sUvLampTimerMutex, MUTEX_TIMEOUT_MS) == pdTRUE)
    {
        float voltage = AdcDriverGetMilliVoltageData(ADC_DRIVER_CHANNEL_UV_1);
        sMeanVoltageUvLamp1 = MeanFilterFilterData(&sFilterUvLamp1, (uint32_t)voltage);
        
        xSemaphoreGive(sUvLampTimerMutex);        
    }

    if (xSemaphoreTake(sUvLampTimerMutex, MUTEX_TIMEOUT_MS) == pdTRUE)
    {
        float voltage = AdcDriverGetMilliVoltageData(ADC_DRIVER_CHANNEL_UV_2);
        sMeanVoltageUvLamp2 = MeanFilterFilterData(&sFilterUvLamp2, (uint32_t)voltage);

        xSemaphoreGive(sUvLampTimerMutex);
    }
}

static bool InitTimer(void)
{
    sUvLampTimerMutex = xSemaphoreCreateMutex();
    if (sUvLampTimerMutex == NULL) {
        ESP_LOGE(TAG, "Mutex Fail");
        return false;
    }

    sTimerHandler = xTimerCreate(sTimerName, TIMER_PERIOD_MS, pdTRUE, (void *) 0, vUvLampTimerCallback);
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

static bool SetMinMaxVoltageLevels(void)
{
    bool res = true;

    res &= FactorySettingsGetServiceParam(FACTORY_SETTING_SERVICE_UV_OFF_MIN_MILIVOLT, &sOffMinInMiliVolt);
    res &= FactorySettingsGetServiceParam(FACTORY_SETTING_SERVICE_UV_OFF_MAX_MILIVOLT, &sOffMaxInMiliVolt);

    res &= FactorySettingsGetServiceParam(FACTORY_SETTING_SERVICE_UV_ON_MIN_MILIVOLT, &sOnMinInMiliVolt);
    res &= FactorySettingsGetServiceParam(FACTORY_SETTING_SERVICE_UV_ON_MAX_MILIVOLT, &sOnMaxInMiliVolt);

    ESP_LOGI(TAG, "off min %u [mV]", sOffMinInMiliVolt);
    ESP_LOGI(TAG, "off max %u [mV]", sOffMaxInMiliVolt);
    ESP_LOGI(TAG, "on min %u [mV]", sOnMinInMiliVolt);
    ESP_LOGI(TAG, "on max %u [mV]", sOnMaxInMiliVolt);

    return res;
}

static bool InitMeanStruct(void)
{   
    bool res = true;

    res &= MeanFilterInit(&sFilterUvLamp1, MEAN_BUFFER_SIZE);
    res &= MeanFilterInit(&sFilterUvLamp2, MEAN_BUFFER_SIZE);

    return res;
}