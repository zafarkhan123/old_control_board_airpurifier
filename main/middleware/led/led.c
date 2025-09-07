/**
 * @file led.c
 *
 * @brief Rgb led middleware source file
 *
 * @dir led
 * @brief Led middleware folder
 *
 * @author matfio
 * @date 2021.08.27
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include <string.h>

#include "config.h"

#include "esp_log.h"

#include "led.h"

#include "ledDriver/ledDriver.h"
#include "gpioExpanderDriver/gpioExpanderDriver.h"
#include "timeDriver/timeDriver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define TIMER_PERIOD_MS (100U)

#define WIFI_TOGGLE_INTERVAL_MS (1U * 1000U)

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

typedef enum{
    LOCK_SEQUENCE_START = 0,
    LOCK_SEQUENCE_LED_OFF_1,
    LOCK_SEQUENCE_LED_ON_1,
    LOCK_SEQUENCE_LED_OFF_2,
    LOCK_SEQUENCE_LED_ON_2,
    LOCK_SEQUENCE_LED_OFF_3,
    LOCK_SEQUENCE_LED_ON_3,
    LOCK_SEQUENCE_STOP,
    LOCK_SEQUENCE_COUNT
}LockSequence_t;

static LockSequence_t sLockSequence = LOCK_SEQUENCE_START;
static TimerHandle_t sTimerHandler;
static StaticTimer_t sTimerBuffer;

static const char* TAG = "led";
static const char* sTimerName = "lockTimer";

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Device is off. All leds are turn off only one red is turn on
 *  @param settingDevice [in] pointer to SettingDevice_t
 *  @return return true if success
 */
static bool DeviceOff(const SettingDevice_t* settingDevice);

/** @brief Device is on. Display device status on leds
 *  @return return true if success
 */
static bool DeviceOn(void);

/** @brief Display fan level.
 *  @param fanLevel fan level
 *  @return return true if success
 */
static bool FanLevel(SettingFanLevel_t fanLevel);

/** @brief Display error information
 *  @param settingDevice [in] pointer to SettingDevice_t
 *  @return return true if success
 */
static bool Error(const SettingDevice_t* settingDevice);

/** @brief Display warning information
 *  @param settingDevice [in] pointer to SettingDevice_t
 *  @return return true if success
 */
static bool Warning(const SettingDevice_t* settingDevice);

/** @brief Set lock led when touch screen is lock
 *  @param settingDevice [in] pointer to SettingDevice_t
 *  @return return true if success
 */
static bool TouchLock(const SettingDevice_t* settingDevice);

/** @brief Free Rtos timer init and start
 *  @return return true if success
 */
static bool InitTimer(void);

/** @brief Calback function. Call when the timer expires
 *  @param pxTimer pointer to TimerHandle_t
 */
static void vLockSequenceTimerCallback(TimerHandle_t pxTimer);

/** @brief Display information if hepa filter and uv lamp starting to wear out
 *  @param settingDevice [in] pointer to SettingDevice_t
 *  @return return true if success
 */
static bool HepafilterAndUvLampWear(const SettingDevice_t* settingDevice);

/** @brief Display information if wifi is connect
 *  @param settingDevice [in] pointer to SettingDevice_t
 *  @return return true if success
 */
static bool Wifi(const SettingDevice_t* settingDevice);

/** @brief Set 3 leds in correct way fan status, inc, dec
 *  @param settingDevice [in] pointer to SettingDevice_t
 *  @return return true if success
 */
static bool FanStatus(const SettingDevice_t* settingDevice);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool LedInit(void)
{
    bool res = true;

    res &= LedDriverInit();
    res &= InitTimer();

    return res;
}

bool LedChangeColor(const SettingDevice_t* settingDevice)
{
    bool res = true;

    if(settingDevice->alarmError.isDetected == true){
        res &= Error(settingDevice);
        res &= TouchLock(settingDevice);
        res &= Wifi(settingDevice);
        res &= LedDriverChangeColor();

        return res;
    }

    if(settingDevice->restore.deviceStatus.isDeviceOn == false)
    {
        res &= DeviceOff(settingDevice);
        res &= TouchLock(settingDevice);
        res &= Wifi(settingDevice);
        res &= Warning(settingDevice);

        res &= LedDriverChangeColor();

        return res;
    }

    res &= DeviceOn();
    res &= FanLevel(settingDevice->restore.deviceStatus.fanLevel);
    res &= FanStatus(settingDevice);
    res &= TouchLock(settingDevice);
    res &= HepafilterAndUvLampWear(settingDevice);
    res &= Wifi(settingDevice);
    res &= Warning(settingDevice);

    res &= LedDriverChangeColor();

    return res;
}

bool LedToggleWifi(const SettingDevice_t* settingDevice)
{
    bool res = true;
    static int64_t sWifiToggleTime;

    if(settingDevice->restore.isWifiOn == false){
        return true;
    }

    if(settingDevice->wifiMode == WIFI_MODE_STA){
        return true;
    }

    if(TimeDriverHasTimeElapsed(sWifiToggleTime, WIFI_TOGGLE_INTERVAL_MS)){
        sWifiToggleTime = TimeDriverGetSystemTickMs();
        static uint8_t sWifiLedOff;

        if(sWifiLedOff != 0){
            res &= LedDriverSetColor(LED_NAME_WIFI_STATUS, LED_COLOR_BLUE);
            sWifiLedOff = 0;
        }
        else{
            res &= LedDriverSetColor(LED_NAME_WIFI_STATUS, LED_COLOR_OFF);
            sWifiLedOff = 1;
        }

        res &= LedDriverChangeColor();
    }
    return res;
}

void LedLockSequenceStart(void)
{
    BaseType_t isActive = xTimerIsTimerActive(sTimerHandler);
    if(isActive == pdTRUE){
        ESP_LOGW(TAG, "is running now");
        return;
    }

    if(sLockSequence != LOCK_SEQUENCE_START){
        ESP_LOGW(TAG, "is performing now");
        return;
    }

    sLockSequence = LOCK_SEQUENCE_START;
    if(xTimerStart(sTimerHandler, 0) != pdPASS){
        ESP_LOGE(TAG, "Start Timer Fail");
        return;
    }
}

bool LedResetFactoryInformation(void)
{
    bool res = true;

    for(uint16_t idx = 0; idx < LED_NAME_COUNT; ++idx)
    {
        res &= LedDriverSetColor(idx, LED_COLOR_BLUE);
    }

    res &= LedDriverChangeColor();
    
    return res;
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool DeviceOff(const SettingDevice_t* settingDevice)
{
    bool res = true;

    for(uint16_t idx = 0; idx < LED_NAME_COUNT; ++idx)
    {
        res &= LedDriverSetColor(idx, LED_COLOR_OFF);
    }

    if(settingDevice->restore.touchLock == false){
        res &= LedDriverSetColor(LED_NAME_PWR, LED_COLOR_RED);
    }
    res &= LedDriverSetColor(LED_NAME_LOGO_OPTIONAL, LED_COLOR_LOGO);
    res &= LedDriverSetColor(LED_NAME_LOGO, LED_COLOR_LOGO);

    return res;
}

static bool DeviceOn(void)
{
    bool res = true;

    res &= LedDriverSetColor(LED_NAME_PWR, LED_COLOR_GREEN);
    res &= LedDriverSetColor(LED_NAME_LOGO_OPTIONAL, LED_COLOR_LOGO);
    res &= LedDriverSetColor(LED_NAME_LOGO, LED_COLOR_LOGO);
    res &= LedDriverSetColor(LED_NAME_HEPA_STATUS, LED_COLOR_GREEN);
    res &= LedDriverSetColor(LED_NAME_UV_STATUS, LED_COLOR_GREEN);
    res &= LedDriverSetColor(LED_NAME_ALARM, LED_COLOR_OFF);

    return res;
}

static bool FanLevel(SettingFanLevel_t fanLevel)
{
    bool res = true;

    switch(fanLevel)
    {
        case FAN_LEVEL_1:{
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_1, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_2, LED_COLOR_WHITE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_3, LED_COLOR_WHITE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_4, LED_COLOR_WHITE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_5, LED_COLOR_WHITE);
            break;
        }
        case FAN_LEVEL_2:{
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_1, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_2, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_3, LED_COLOR_WHITE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_4, LED_COLOR_WHITE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_5, LED_COLOR_WHITE);
            break;
        }
        case FAN_LEVEL_3:{
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_1, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_2, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_3, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_4, LED_COLOR_WHITE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_5, LED_COLOR_WHITE);
            break;
        }
        case FAN_LEVEL_4:{
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_1, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_2, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_3, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_4, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_5, LED_COLOR_WHITE);
            break;
        }
        case FAN_LEVEL_5:{
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_1, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_2, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_3, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_4, LED_COLOR_BLUE);
            res &= LedDriverSetColor(LED_NAME_FAN_SPEED_LEVEL_5, LED_COLOR_BLUE);
            break;
        }
        case FAN_LEVEL_COUNT:{
            break;
        }
    }

    return res;
}

static bool Error(const SettingDevice_t* settingDevice)
{
    bool res = true;

    // turn off all leds
    for(uint16_t idx = 0; idx < LED_NAME_COUNT;  ++idx)
    {
        res &= LedDriverSetColor(idx, LED_COLOR_OFF);
    }

    res &= LedDriverSetColor(LED_NAME_PWR, LED_COLOR_ORANGE);
    res &= LedDriverSetColor(LED_NAME_LOGO_OPTIONAL, LED_COLOR_LOGO);
    res &= LedDriverSetColor(LED_NAME_LOGO, LED_COLOR_LOGO);

    if(settingDevice->restore.touchLock == true){
        res &= LedDriverSetColor(LED_NAME_LOCK, LED_COLOR_ORANGE);
    }else{
        res &= LedDriverSetColor(LED_NAME_LOCK, LED_COLOR_OFF);
    }

    if(settingDevice->alarmError.fanSpeed == true){
        res &= LedDriverSetColor(LED_NAME_FAN_STATUS, LED_COLOR_RED);
    }

    if((settingDevice->alarmError.uvLampBallast1 == true) || (settingDevice->alarmError.uvLampBallast2 == true) ||
    (settingDevice->alarmError.stuckRelayUvLamp1 == true) || (settingDevice->alarmError.stuckRelayUvLamp2 == true)){
        res &= LedDriverSetColor(LED_NAME_UV_STATUS, LED_COLOR_RED);
    }

    if((settingDevice->alarmError.hepa1Filter == true) || (settingDevice->alarmError.hepa2Filter == true)){
        res &= LedDriverSetColor(LED_NAME_HEPA_STATUS, LED_COLOR_RED);
    }

    res &= LedDriverSetColor(LED_NAME_ALARM, LED_COLOR_RED);

    return res;
}

static bool Warning(const SettingDevice_t* settingDevice)
{
    bool res = true;

    if(settingDevice->alarmWarning.isDetected == false){
        return res;
    }

    res &= LedDriverSetColor(LED_NAME_ALARM, LED_COLOR_RED);

    return res;
}

static bool TouchLock(const SettingDevice_t* settingDevice)
{
    bool res  = true;

    if(settingDevice->restore.touchLock == true){
        res &= LedDriverSetColor(LED_NAME_LOCK, LED_COLOR_ORANGE);
    }else{
        res &= LedDriverSetColor(LED_NAME_LOCK, LED_COLOR_OFF);
    }

    return res;
}

static bool InitTimer(void)
{
    sTimerHandler = xTimerCreateStatic(sTimerName, TIMER_PERIOD_MS, pdTRUE, (void *) 0, vLockSequenceTimerCallback, &sTimerBuffer);
    if(sTimerHandler == NULL){
        ESP_LOGE(TAG, "Can't allocate heap");
        return false;
    }

    return true;
}

static void vLockSequenceTimerCallback(TimerHandle_t pxTimer)
{
    switch(sLockSequence)
    {
        case LOCK_SEQUENCE_START:{
            ESP_LOGI(TAG, "lock seq start");
            GpioExpanderDriverBuzzerOn();
            ++sLockSequence;
            break;
        }
        case LOCK_SEQUENCE_LED_OFF_1:{
            ++sLockSequence;
            LedDriverSetColor(LED_NAME_LOCK, LED_COLOR_OFF);
            LedDriverChangeColor();
            break;
        }
        case LOCK_SEQUENCE_LED_ON_1:{
            ++sLockSequence;
            LedDriverSetColor(LED_NAME_LOCK, LED_COLOR_ORANGE);
            LedDriverChangeColor();
            break;           
        }
        case LOCK_SEQUENCE_LED_OFF_2:{
            ++sLockSequence;
            LedDriverSetColor(LED_NAME_LOCK, LED_COLOR_OFF);
            LedDriverChangeColor();
            break;           
        }
        case LOCK_SEQUENCE_LED_ON_2:{
            ++sLockSequence;
            LedDriverSetColor(LED_NAME_LOCK, LED_COLOR_ORANGE);
            LedDriverChangeColor();
            break;           
        }
        case LOCK_SEQUENCE_LED_OFF_3:{
            ++sLockSequence;
            LedDriverSetColor(LED_NAME_LOCK, LED_COLOR_OFF);
            LedDriverChangeColor();
            break;           
        }
        case LOCK_SEQUENCE_LED_ON_3:{
            ++sLockSequence;
            LedDriverSetColor(LED_NAME_LOCK, LED_COLOR_ORANGE);
            LedDriverChangeColor();
            break;           
        }
        case LOCK_SEQUENCE_STOP:
        default:{
            GpioExpanderDriverBuzzerOff();
            ESP_LOGI(TAG, "lock seq stop");
            BaseType_t isStop = xTimerStop(pxTimer, 0);
            // pdFAIL will be returned if the stop command could not be sent to the timer command queue even after xTicksToWait ticks had passed;
            if(isStop == pdTRUE){
                sLockSequence = LOCK_SEQUENCE_START;
            }
            
            break; 
        }          
    }
}

static bool HepafilterAndUvLampWear(const SettingDevice_t* settingDevice)
{
    bool res = true;
    
    if(settingDevice->timersStatus.isWornOutDetected == true){
        if(settingDevice->timersStatus.hepaFilterReplacementReminder == true){
            res &= LedDriverSetColor(LED_NAME_HEPA_STATUS, LED_COLOR_ORANGE);
        }

        if(settingDevice->timersStatus.hepaFilterLifeTimeExpired == true){
            res &= LedDriverSetColor(LED_NAME_HEPA_STATUS, LED_COLOR_RED);
        }

        if((settingDevice->timersStatus.uvLamp1ReplacementReminder == true) || (settingDevice->timersStatus.uvLamp2ReplacementReminder == true)){
            res &= LedDriverSetColor(LED_NAME_UV_STATUS, LED_COLOR_ORANGE);
        }

        if((settingDevice->timersStatus.uvLamp1LifeTimeExpired == true) || (settingDevice->timersStatus.uvLamp2LifeTimeExpired == true)){
            res &= LedDriverSetColor(LED_NAME_UV_STATUS, LED_COLOR_RED);
        }
    }

    return res;
}

static bool Wifi(const SettingDevice_t* settingDevice)
{
    bool res = true;

    if(settingDevice->restore.isWifiOn == false){
        res &= LedDriverSetColor(LED_NAME_WIFI_STATUS, LED_COLOR_OFF);
    }else{
        if(settingDevice->wifiMode == WIFI_MODE_APSTA)
        {
            if(settingDevice->tryConnectToNewAp == true){
                res &= LedDriverSetColor(LED_NAME_WIFI_STATUS, LED_COLOR_BLUE);
            }
            else{
                res &= LedDriverSetColor(LED_NAME_WIFI_STATUS, LED_COLOR_GREEN);
            }
            
        }else if(settingDevice->wifiMode == WIFI_MODE_STA)
        {
            res &= LedDriverSetColor(LED_NAME_WIFI_STATUS, LED_COLOR_BLUE);
        }else
        {
            res &= LedDriverSetColor(LED_NAME_WIFI_STATUS, LED_COLOR_OFF);
        } 
    }

    return res;
}

static bool FanStatus(const SettingDevice_t* settingDevice)
{
    bool res = true;

    if(settingDevice->restore.touchLock == true){
        res &= LedDriverSetColor(LED_NAME_FAN_SPEED_INCREASE, LED_COLOR_OFF);
        res &= LedDriverSetColor(LED_NAME_FAN_STATUS, LED_COLOR_OFF);
        res &= LedDriverSetColor(LED_NAME_FAN_SPEED_DECREASE, LED_COLOR_OFF);
    }
    else{
        res &= LedDriverSetColor(LED_NAME_FAN_SPEED_INCREASE, LED_COLOR_WHITE);
        res &= LedDriverSetColor(LED_NAME_FAN_STATUS, LED_COLOR_WHITE);
        res &= LedDriverSetColor(LED_NAME_FAN_SPEED_DECREASE, LED_COLOR_WHITE);
    }

    return res;
}