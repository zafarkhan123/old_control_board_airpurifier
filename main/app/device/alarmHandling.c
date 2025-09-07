/**
 * @file alarmHandling.c
 *
 * @brief Alarm handling header source file
 *
 * @dir device
 * @brief Device folder
 *
 * @author matfio
 * @date 2021.11.03
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "alarmHandling.h"

#include "esp_log.h"

#include "uvLamp/uvLamp.h"
#include "fanDriver/fanDriver.h"
#include "rtcDriver/rtcDriver.h"
#include "nvsDriver/nvsDriver.h"
#include "factorySettingsDriver/factorySettingsDriver.h"
#include "timerDriver/timerDriver.h"
#include "timeDriver/timeDriver.h"

#include "fan/fan.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
*****************************************************************************/

#define PREFILTER_ALARM_SWITCH_BUZZER_TIME_MS (1000U)

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static const char* TAG = "errHand";

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool AlarmHandlingErrorCheck(SettingDevice_t* setting, GpioExpanderPinout_t *expanderInput)
{
    bool isErrorDetect = false;

    uvLampStatus_t uvLamp1Status = UvLampDriverGetUvLampStatus(UV_LAMP_1);
    uvLampStatus_t uvLamp2Status = UvLampDriverGetUvLampStatus(UV_LAMP_2);

    if((uvLamp1Status == UV_LAMP_STATUS_ERROR) || ((uvLamp1Status == UV_LAMP_STATUS_OFF) && (setting->uvLamp1On == true))){
        setting->alarmError.uvLampBallast1 = true;
    }
    isErrorDetect |= setting->alarmError.uvLampBallast1;


    if((uvLamp2Status == UV_LAMP_STATUS_ERROR) || ((uvLamp2Status == UV_LAMP_STATUS_OFF) && (setting->uvLamp2On == true))){
        setting->alarmError.uvLampBallast2 = true;
    }
    isErrorDetect |= setting->alarmError.uvLampBallast2;

    if((uvLamp1Status == UV_LAMP_STATUS_ON) && (setting->uvLamp1On == false)){
        setting->alarmError.stuckRelayUvLamp1 = true;
    }
    isErrorDetect |= setting->alarmError.stuckRelayUvLamp1;

    if((uvLamp2Status == UV_LAMP_STATUS_ON) && (setting->uvLamp2On == false)){
        setting->alarmError.stuckRelayUvLamp2 = true;
    }
    isErrorDetect |= setting->alarmError.stuckRelayUvLamp2;

    if(expanderInput->limitSwitch1 == true){
        isErrorDetect = true;
        setting->alarmError.hepa1Filter = true;
    }else{
        setting->alarmError.hepa1Filter = false;
    }

    if( expanderInput->limitSwitch2 == true){
        isErrorDetect = true;
        setting->alarmError.hepa2Filter = true;
    }else{
        setting->alarmError.hepa2Filter = false;
    }

    if( expanderInput->limitSwitch3 == true){
        isErrorDetect = true;
        setting->alarmError.preFilter = true;
    }else{
        setting->alarmError.preFilter = false;
    }

    int16_t count = 0 ;
    FanTachoState_t status = FanGetTachoRevolutionsPerSecond(&count);
    if((setting->restore.deviceStatus.isDeviceOn == true) && (status == FAN_TACHO_WORKS) && (count == 0)){
        setting->alarmError.fanSpeed = true;
        // lock fan error. Clear when someone press on/off button
    }
    isErrorDetect |= setting->alarmError.fanSpeed;

    setting->alarmError.isDetected = isErrorDetect;

    return isErrorDetect;
}

bool AlarmHandlingWarningCheck(SettingDevice_t* setting)
{
    bool isWarningDetect = false;

    if(SettingIsError() == true){
        isWarningDetect = true;
        setting->alarmWarning.memory = true;
    }else{
        setting->alarmWarning.memory = false;
    }

    if(RtcDriverIsError() == true){
        isWarningDetect = true;
        setting->alarmWarning.rtc = true;
    }else{
        setting->alarmWarning.rtc = false;
    }

    setting->alarmWarning.isDetected = isWarningDetect;

    return isWarningDetect;
}

bool AlarmHandlingTimersWornOutCheck(SettingDevice_t* setting)
{
    bool isWornOutDetected = false;
    uint32_t serviceSetting = 0;

    uint64_t liveTime = TIMER_DRIVER_RAW_DATA_TO_HOUR(setting->restore.liveTime[TIMER_NAME_HEPA]);

    FactorySettingsGetServiceParam(FACTORY_SETTING_SERVICE_HEPA_WARNING_HOURS, &serviceSetting);
    if(liveTime >= serviceSetting){
        ESP_LOGI(TAG, "hepa reminder pass %llu > %u", liveTime, serviceSetting);
        isWornOutDetected = true;
        setting->timersStatus.hepaFilterReplacementReminder = true;
    }else{
        setting->timersStatus.hepaFilterReplacementReminder = false;
    }

    FactorySettingsGetServiceParam(FACTORY_SETTING_SERVICE_HEPA_LIFETIME_HOURS, &serviceSetting);
    if(liveTime >= serviceSetting){
        ESP_LOGI(TAG, "hepa lifetime pass %llu > %u", liveTime, serviceSetting);
        isWornOutDetected = true;
        setting->timersStatus.hepaFilterLifeTimeExpired = true;
    }else{
        setting->timersStatus.hepaFilterLifeTimeExpired = false;
    }

    liveTime = TIMER_DRIVER_RAW_DATA_TO_HOUR(setting->restore.liveTime[TIMER_NAME_UV_LAMP_1]);

    FactorySettingsGetServiceParam(FACTORY_SETTING_SERVICE_UV_WARNING_HOURS, &serviceSetting);
    if(liveTime >= serviceSetting){
        ESP_LOGI(TAG, "uv1 reminder pass %llu > %u", liveTime, serviceSetting);
        isWornOutDetected = true;
        setting->timersStatus.uvLamp1ReplacementReminder = true;
    }else{
        setting->timersStatus.uvLamp1ReplacementReminder = false;
    }

    FactorySettingsGetServiceParam(FACTORY_SETTING_SERVICE_UV_LIFETIME_HOURS, &serviceSetting);
    if(liveTime >= serviceSetting){
        ESP_LOGI(TAG, "uv1 lifetime pass %llu > %u", liveTime, serviceSetting);
        isWornOutDetected = true;
        setting->timersStatus.uvLamp1LifeTimeExpired = true;
    }else{
        setting->timersStatus.uvLamp1LifeTimeExpired = false;
    }

    liveTime = TIMER_DRIVER_RAW_DATA_TO_HOUR(setting->restore.liveTime[TIMER_NAME_UV_LAMP_2]);

    FactorySettingsGetServiceParam(FACTORY_SETTING_SERVICE_UV_WARNING_HOURS, &serviceSetting);
    if(liveTime >= serviceSetting){
        ESP_LOGI(TAG, "uv2 reminder pass %llu > %u", liveTime, serviceSetting);
        isWornOutDetected = true;
        setting->timersStatus.uvLamp2ReplacementReminder = true;
    }else{
        setting->timersStatus.uvLamp2ReplacementReminder = false;
    }

    FactorySettingsGetServiceParam(FACTORY_SETTING_SERVICE_UV_LIFETIME_HOURS, &serviceSetting);
    if(liveTime >= serviceSetting){
        ESP_LOGI(TAG, "uv2 lifetime pass %llu > %u", liveTime, serviceSetting);
        isWornOutDetected = true;
        setting->timersStatus.uvLamp2LifeTimeExpired = true;
    }else{
        setting->timersStatus.uvLamp2LifeTimeExpired = false;
    }

    setting->timersStatus.isWornOutDetected = isWornOutDetected;

    return isWornOutDetected;
}

void AlarmHandlingPrint(const SettingDevice_t* setting)
{
    if(setting->alarmError.isDetected == true){
        ESP_LOGE(TAG, "error detected");
        ESP_LOGE(TAG, "uv lamp ballast %d, %d", setting->alarmError.uvLampBallast1, setting->alarmError.uvLampBallast2);
        ESP_LOGE(TAG, "pre filter %d", setting->alarmError.preFilter);
        ESP_LOGE(TAG, "hepa filter %d, %d", setting->alarmError.hepa1Filter, setting->alarmError.hepa2Filter);
        ESP_LOGE(TAG, "fan speed %d", setting->alarmError.fanSpeed);
        ESP_LOGE(TAG, "stuck relay %d, %d", setting->alarmError.stuckRelayUvLamp1, setting->alarmError.stuckRelayUvLamp2);
    }else{
        ESP_LOGI(TAG, "no error detected");
    }

    if(setting->alarmWarning.isDetected == true){
        ESP_LOGW(TAG, "warning detected");
        ESP_LOGW(TAG, "memory %d", setting->alarmWarning.memory);
        ESP_LOGW(TAG, "rtc %d", setting->alarmWarning.rtc);
    }else{
        ESP_LOGI(TAG, "no warning detected\n");
    }

    if(setting->timersStatus.isWornOutDetected == true){
        ESP_LOGW(TAG, "hepa reminder %d", setting->timersStatus.hepaFilterReplacementReminder);
        ESP_LOGW(TAG, "hepa life time expired %d", setting->timersStatus.hepaFilterLifeTimeExpired);

        ESP_LOGW(TAG, "uv 1 reminder %d", setting->timersStatus.uvLamp1ReplacementReminder);
        ESP_LOGW(TAG, "uv 1 life time expired %d", setting->timersStatus.uvLamp1LifeTimeExpired);
            
        ESP_LOGW(TAG, "uv 2 reminder %d", setting->timersStatus.uvLamp2ReplacementReminder);
        ESP_LOGW(TAG, "uv 2 life time expired %d", setting->timersStatus.uvLamp2LifeTimeExpired);
    }else{
        ESP_LOGI(TAG, "hepa and uv lamp are ok");
    }
}

void AlarmHandlingManagement(SettingDevice_t* setting)
{
    if(setting->alarmError.isDetected == true){
        setting->restore.deviceStatus.isDeviceOn = false;
        setting->restore.deviceStatus.fanLevel = FAN_LEVEL_1;
        setting->uvLamp1On = false;
        setting->uvLamp2On = false;

        if(setting->alarmError.preFilter == true){
            static int64_t buzzerSwitchTime = 0;
            if(TimeDriverHasTimeElapsed(buzzerSwitchTime, PREFILTER_ALARM_SWITCH_BUZZER_TIME_MS)){
                buzzerSwitchTime = TimeDriverGetSystemTickMs();

                if(GpioExpanderDriverIsBuzzerOn() == true){
                    GpioExpanderDriverBuzzerOff();
                }
                else{
                    GpioExpanderDriverBuzzerOn();
                }
            }
        }
        else{
            if(GpioExpanderDriverIsBuzzerOn() == false){
                GpioExpanderDriverBuzzerOn();
                ESP_LOGI(TAG, "Buzzer on");
                AlarmHandlingPrint(setting);
            }        
        }
    }
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/