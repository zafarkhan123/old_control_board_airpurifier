/**
 * @file messageType.c
 *
 * @brief message Type used in http comunication
 *
 * @dir webServer
 * @brief webServer middleware folder
 *
 * @author matfio
 * @date 2021.09.01
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "messageType.h"

#include "esp_ota_ops.h"

#include <esp_log.h>

#include "device/alarmHandling.h"

#include "timerDriver/timerDriver.h"
#include "factorySettingsDriver/factorySettingsDriver.h"
#include "timeDriver/timeDriver.h"
#include "rtcDriver/rtcDriver.h"

#include "uvLamp/uvLamp.h"
#include "fan/fan.h"

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static const char *TAG = "messageT";

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Add alarm code to array which will then be sent
 *  @param status [out] pointer to messageTypeDeviceStatusHttpClient_t
 *  @param code [in] code number
 */
static bool AddAlarmCodeToArray(messageTypeDeviceStatusHttpClient_t* status, uint8_t code);

/** @brief Check device status and add alarm code to struct messageTypeDeviceStatusHttpClient_t
 *  @param deviceStatus [out] pointer to messageTypeDeviceStatusHttpClient_t
 *  @param setting [in] pointer to SettingDevice_t
 */
static bool CheckErrorAndWarningAndPrepareToSend(messageTypeDeviceStatusHttpClient_t* deviceStatus, const SettingDevice_t* setting);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

void MessageTypeCreateDeviceInfo(messageTypeDeviceInfo_t* deviceInfo, const SettingDevice_t* setting)
{   
    deviceInfo->Switch = setting->restore.deviceStatus.isDeviceOn;
    deviceInfo->fan = setting->restore.deviceStatus.fanLevel;

    deviceInfo->lamp = MessageTypeGetUvLampLifeTimeInPercent(setting->restore.liveTime[TIMER_NAME_UV_LAMP_1]);
    deviceInfo->hepa = MessageTypeGetHepaLifeTimeInPercent(setting->restore.liveTime[TIMER_NAME_HEPA]);
    
    const esp_app_desc_t *app_desc = esp_ota_get_app_description();
    deviceInfo->sw_version = (char*)app_desc->version;
    deviceInfo->compile_date = (char*)app_desc->date;
    deviceInfo->compile_time = (char*)app_desc->time;

    deviceInfo->automatical = setting->restore.deviceMode;
    deviceInfo->wifiConnect = (setting->wifiStatus == WIFI_STATUS_STA_CONNECTED);

    deviceInfo->ecoOn = setting->restore.deviceStatus.isEkoOn;
    deviceInfo->lockOn = setting->restore.touchLock;
    
    deviceInfo->timestamp = TimeDriverGetUTCUnixTime();
}   

void MessageTypeCreateSettingDevice(messageTypeDeviceMode_t* mode, SettingDevice_t* setting)
{
    setting->restore.deviceStatus.isDeviceOn = mode->Switch;
    if(mode->fan >= FAN_LEVEL_COUNT){
        ESP_LOGW(TAG, "incorrect fan level %u", mode->fan);

        mode->fan = FAN_LEVEL_1;
    }

    setting->restore.deviceStatus.fanLevel = mode->fan;
    setting->restore.deviceMode = mode->automatical;
    setting->restore.deviceStatus.isEkoOn = mode->ecoOn;
    setting->restore.touchLock = mode->lockOn;
}

void MessageTypeCreateScheduler(const messageTypeScheduler_t* message, Scheduler_t* scheduler)
{
    for(uint16_t dayIdx = 0; dayIdx < SCHEDULER_DAY_COUNT; ++dayIdx){
        for(uint16_t hourIdx = 0; hourIdx < SCHEDULER_HOUR_COUNT; ++hourIdx){

            uint8_t setting = message->deviceSetting[dayIdx][hourIdx].setting;
            switch (setting)
            {
                case 0:{
                    scheduler->days[dayIdx].hours[hourIdx].isDeviceOn = false;
                    scheduler->days[dayIdx].hours[hourIdx].fanLevel = 0;
                    break;
                }
                case 1:{
                    scheduler->days[dayIdx].hours[hourIdx].isDeviceOn = true;
                    scheduler->days[dayIdx].hours[hourIdx].fanLevel = 0;
                    break;
                }
                case 2:{
                    scheduler->days[dayIdx].hours[hourIdx].isDeviceOn = true;
                    scheduler->days[dayIdx].hours[hourIdx].fanLevel = 1;
                    break;
                }
                case 3:{
                    scheduler->days[dayIdx].hours[hourIdx].isDeviceOn = true;
                    scheduler->days[dayIdx].hours[hourIdx].fanLevel = 2;
                    break;
                }
                case 4:{
                    scheduler->days[dayIdx].hours[hourIdx].isDeviceOn = true;
                    scheduler->days[dayIdx].hours[hourIdx].fanLevel = 3;
                    break;
                }
                case 5:{
                    scheduler->days[dayIdx].hours[hourIdx].isDeviceOn = true;
                    scheduler->days[dayIdx].hours[hourIdx].fanLevel = 4;
                    break;
                }
                default:{
                    ESP_LOGE(TAG, "create Scheduler_t incorrect value %d", setting);
                    break;
                }
            }
            scheduler->days[dayIdx].hours[hourIdx].isEkoOn = message->deviceSetting[dayIdx][hourIdx].isEco;
        }
    }
}

void MessageTypeCreateMessageTypeScheduler(messageTypeScheduler_t* message, const Scheduler_t* scheduler)
{
    message->timestamp = TimeDriverGetUTCUnixTime();
    for(uint16_t dayIdx = 0; dayIdx < SCHEDULER_DAY_COUNT; ++dayIdx){
        for(uint16_t hourIdx = 0; hourIdx < SCHEDULER_HOUR_COUNT; ++hourIdx){
            if(scheduler->days[dayIdx].hours[hourIdx].isDeviceOn == false){
                message->deviceSetting[dayIdx][hourIdx].setting = 0;
            }
            else{
                switch(scheduler->days[dayIdx].hours[hourIdx].fanLevel){
                    case 0:{
                        message->deviceSetting[dayIdx][hourIdx].setting = 1;
                        break;
                    }
                    case 1:{
                        message->deviceSetting[dayIdx][hourIdx].setting = 2;
                        break;
                    }
                    case 2:{
                        message->deviceSetting[dayIdx][hourIdx].setting = 3;
                        break;
                    }
                    case 3:{
                        message->deviceSetting[dayIdx][hourIdx].setting = 4;
                        break;
                    }
                    case 4:{
                        message->deviceSetting[dayIdx][hourIdx].setting = 5;
                        break;
                    }
                    default:{
                        ESP_LOGE(TAG, "create messageTypeScheduler_t incorrect value %d", scheduler->days[dayIdx].hours[hourIdx].fanLevel);
                        break;
                    }
                }
            }
            message->deviceSetting[dayIdx][hourIdx].isEco = scheduler->days[dayIdx].hours[hourIdx].isEkoOn;
        }
    }
}

void MessageTypeCreateDeviceInfoHttpClient(messageTypeDeviceInfoHttpClient_t* deviceInfo)
{    
    deviceInfo->hwVersion = FactorySettingsGetHardwareVersion();

    const esp_app_desc_t *app_desc = esp_ota_get_app_description();
    deviceInfo->swVersion = (char*)app_desc->version;
}

void MessageTypeCreateDeviceStatusHttpClient(messageTypeDeviceStatusHttpClient_t* deviceStatus, const SettingDevice_t* setting)
{
    deviceStatus->timestamp = TimeDriverGetUTCUnixTime();
    deviceStatus->mode = setting->restore.deviceMode;
    deviceStatus->totalOn = TIMER_DRIVER_RAW_DATA_TO_HOUR(setting->restore.liveTime[TIMER_NAME_GLOBAL_ON]);

    deviceStatus->timUv1 = TIMER_DRIVER_RAW_DATA_TO_HOUR(setting->restore.liveTime[TIMER_NAME_UV_LAMP_1]);
    deviceStatus->timUv2 = TIMER_DRIVER_RAW_DATA_TO_HOUR(setting->restore.liveTime[TIMER_NAME_UV_LAMP_2]);

    deviceStatus->timHepa = TIMER_DRIVER_RAW_DATA_TO_HOUR(setting->restore.liveTime[TIMER_NAME_HEPA]);

    struct tm rtcTime = {};

    RtcDriverGetDateTime(&rtcTime);
    time_t rtcUnix = mktime(&rtcTime);

    deviceStatus->rtc = rtcUnix;

    if(setting->restore.deviceStatus.isDeviceOn == false){
        deviceStatus->fanLevel = 0;
    }
    else{
        deviceStatus->fanLevel = setting->restore.deviceStatus.fanLevel + 1;
    }
    
    deviceStatus->isEco = setting->restore.deviceStatus.isEkoOn;

    bool result =  CheckErrorAndWarningAndPrepareToSend(deviceStatus, setting);
    if(result == false){
        ESP_LOGE(TAG, "error code not enough space");
    }
    if (setting->ethernetPcbAdded == false){
        deviceStatus->ethernetOn = false;
    }else{
        deviceStatus->ethernetOn = (setting->ethernetStatus == ETHERNET_EVENT_CONNECTED);
    }
    
    deviceStatus->touchLock = setting->restore.touchLock;
    deviceStatus->wifiOn = (setting->wifiStatus == WIFI_STATUS_STA_CONNECTED);

    deviceStatus->resetReason = esp_reset_reason();

    static bool addOnlyOnce = false;
    if(addOnlyOnce == false){
        deviceStatus->deviceReset = setting->newFirmwareVeryfication;
        addOnlyOnce = true;
    }
}

void MessageTypeCreateSettingFromDeviceModeHttpClient(const messageTypeDeviceModeHttpClient_t* deviceMode, SettingDevice_t* setting)
{
    setting->restore.deviceMode = deviceMode->mode;

    uint8_t deviceSetting = deviceMode->fanLevel;

    switch(deviceSetting)
    {
        case (0):{
            setting->restore.deviceStatus.isDeviceOn = false;
            setting->restore.deviceStatus.fanLevel = FAN_LEVEL_1;
            break;
        }

        case (1):{
            setting->restore.deviceStatus.isDeviceOn = true;
            setting->restore.deviceStatus.fanLevel = FAN_LEVEL_1;
            break;
        }

        case (2):{
            setting->restore.deviceStatus.isDeviceOn = true;
            setting->restore.deviceStatus.fanLevel = FAN_LEVEL_2;
            break;
        }

        case (3):{
            setting->restore.deviceStatus.isDeviceOn = true;
            setting->restore.deviceStatus.fanLevel = FAN_LEVEL_3;
            break;
        }

        case (4):{
            setting->restore.deviceStatus.isDeviceOn = true;
            setting->restore.deviceStatus.fanLevel = FAN_LEVEL_4;
            break;
        }

        case (5):{
            setting->restore.deviceStatus.isDeviceOn = true;
            setting->restore.deviceStatus.fanLevel = FAN_LEVEL_5;
            break;
        }

        default:{
            setting->restore.deviceStatus.isDeviceOn = false;
            setting->restore.deviceStatus.fanLevel = FAN_LEVEL_1;

            ESP_LOGE(TAG, "messageTypeDeviceModeHttpClient_t incorrtct value %d in fanLevel", deviceSetting);
        }
    }

    setting->restore.deviceStatus.isEkoOn = deviceMode->isEco;
    setting->restore.touchLock = deviceMode->touchLock;
}

void MessageTypeCreateDeviceModeHttpClientFromSetting(messageTypeDeviceModeHttpClient_t* deviceMode, const SettingDevice_t* setting)
{
    deviceMode->mode = setting->restore.deviceMode;
    deviceMode->isEco = setting->restore.deviceStatus.isEkoOn;
    deviceMode->touchLock = setting->restore.touchLock;

    if(setting->restore.deviceStatus.isDeviceOn == false)
    {
        deviceMode->fanLevel = 0;
    }else{
        switch (setting->restore.deviceStatus.fanLevel)
        {
        case FAN_LEVEL_1:
            deviceMode->fanLevel = 1;
            break;
        
        case FAN_LEVEL_2:
            deviceMode->fanLevel = 2;
            break;
                
        case FAN_LEVEL_3:
            deviceMode->fanLevel = 3;
            break;
                
        case FAN_LEVEL_4:
            deviceMode->fanLevel = 4;
            break;

        case FAN_LEVEL_5:
            deviceMode->fanLevel = 5;
            break;

        default:
            deviceMode->fanLevel = 0;
            break;
        }
    }
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool AddAlarmCodeToArray(messageTypeDeviceStatusHttpClient_t* status, uint8_t code)
{
    if(status->alarmCodeIdx < MESSAGE_TYPE_ALARM_CODE_ARRAY_LEN){
        status->alarmCode[status->alarmCodeIdx] = code;

        ++status->alarmCodeIdx;

        return true;
    }

    return false;
}

bool CheckErrorAndWarningAndPrepareToSend(messageTypeDeviceStatusHttpClient_t* deviceStatus, const SettingDevice_t* setting)
{
    bool result = true;

    esp_reset_reason_t reason = esp_reset_reason();
    if(reason == ESP_RST_BROWNOUT){
        result = AddAlarmCodeToArray(deviceStatus, ERROR_CODE_POWER_OFF);
        if(result == false){
            return false;
        }
    }

    if(setting->alarmWarning.isDetected == true){
        if(setting->alarmWarning.rtc == true){
            result = AddAlarmCodeToArray(deviceStatus, ERROR_CODE_DATE_TIME_ERROR);
            if(result == false){
                return false;
            }
        }

        if(setting->alarmWarning.memory == true){
            result = AddAlarmCodeToArray(deviceStatus, ERROR_CODE_INTERNAL_MEMORY_ERROR);
            if(result == false){
                return false;
            }
        }
    }

    if(setting->alarmError.isDetected == true){
        if(setting->alarmError.preFilter == true){
            result = AddAlarmCodeToArray(deviceStatus, ERROR_CODE_PRE_FILTER_CICRUIT_OPEN);
            if(result == false){
                return false;
            }
        }

        if(setting->alarmError.hepa1Filter == true){
            result = AddAlarmCodeToArray(deviceStatus, ERROR_CODE_HEPA_1_FILTER_LIMIT_SWITCH);
            if(result == false){
                return false;
            }
        }

        if(setting->alarmError.hepa2Filter == true){
            result = AddAlarmCodeToArray(deviceStatus, ERROR_CODE_HEPA_2_FILTER_LIMIT_SWITCH);
            if(result == false){
                return false;
            }
        }

        if(setting->alarmError.uvLampBallast1 == true){
            result = AddAlarmCodeToArray(deviceStatus, ERROR_CODE_UV_1_POWER_CIRCUIT_FAULT);
            if(result == false){
                return false;
            }
        }

        if(setting->alarmError.uvLampBallast2 == true){
            result = AddAlarmCodeToArray(deviceStatus, ERROR_CODE_UV_2_POWER_CIRCUIT_FAULT);
            if(result == false){
                return false;
            }
        }

        if(setting->alarmError.fanSpeed == true){
            result = AddAlarmCodeToArray(deviceStatus, ERROR_CODE_FAN_CIRCUIT_FAULT);
            if(result == false){
                return false;
            }
        }

        if((setting->alarmError.uvLampBallast1 == true) || (setting->alarmError.uvLampBallast2 == true) ||
        (setting->alarmError.stuckRelayUvLamp1 == true) || (setting->alarmError.stuckRelayUvLamp2 == true)){
            result = AddAlarmCodeToArray(deviceStatus, ERROR_CODE_UV_LAMPS_CONTROL_ERROR);
            if(result == false){
                return false;
            }
        }
    }

    if(setting->timersStatus.isWornOutDetected == true){
        if(setting->timersStatus.hepaFilterLifeTimeExpired == true){
            result = AddAlarmCodeToArray(deviceStatus, ERROR_CODE_FILTER_SERVICE_LIFE_EXCEEDED);
            if(result == false){
                return false;
            }
        }

        if((setting->timersStatus.uvLamp1LifeTimeExpired == true) || (setting->timersStatus.uvLamp2LifeTimeExpired == true)){
            result = AddAlarmCodeToArray(deviceStatus, ERROR_CODE_UV_LAMPS_SERVICE_LIFE_EXCEEDED);
            if(result == false){
                return false;
            }
        }

        if((setting->timersStatus.hepaFilterReplacementReminder == true) && (setting->timersStatus.hepaFilterLifeTimeExpired == false)){
            result = AddAlarmCodeToArray(deviceStatus, WARNING_CODE_HEPA_FILTER_CHANGE_REMAINDER);
            if(result == false){
                return false;
            }
        }

        if(((setting->timersStatus.uvLamp1ReplacementReminder == true) && (setting->timersStatus.uvLamp1LifeTimeExpired == false)) 
        || ((setting->timersStatus.uvLamp2ReplacementReminder == true) && ((setting->timersStatus.uvLamp2LifeTimeExpired == false)))){
            result = AddAlarmCodeToArray(deviceStatus, WARNING_CODE_UV_LAMP_CHANGE_REMAINDER);
            if(result == false){
                return false;
            }
        }

    }

    return result;
}

uint16_t MessageTypeGetHepaLifeTimeInPercent(uint64_t actualLiveTime)
{
    uint32_t lifeTime = CFG_HEPA_SERVICE_LIFETIME_HOURS;
    FactorySettingsGetServiceParam(FACTORY_SETTING_SERVICE_HEPA_LIFETIME_HOURS, &lifeTime);

    int64_t serviceLifePercent = 100ULL - (100ULL * TIMER_DRIVER_RAW_DATA_TO_HOUR(actualLiveTime)) / lifeTime;
    
    if(serviceLifePercent > 100){
        serviceLifePercent = 100;
    }
    
    if(serviceLifePercent < 0){
        serviceLifePercent = 0;
    }

    return (uint16_t)serviceLifePercent;
}

uint16_t MessageTypeGetUvLampLifeTimeInPercent(uint64_t actualLiveTime)
{
    uint32_t lifeTime = CFG_UV_LAMP_SERVICE_LIFETIME_HOURS;
    FactorySettingsGetServiceParam(FACTORY_SETTING_SERVICE_UV_LIFETIME_HOURS, &lifeTime);

    int64_t serviceLifePercent = 100ULL - (100ULL * TIMER_DRIVER_RAW_DATA_TO_HOUR(actualLiveTime)) / lifeTime;

    if(serviceLifePercent > 100){
        serviceLifePercent = 100;
    }

    if(serviceLifePercent < 0){
        serviceLifePercent = 0;
    }
    
    return (uint16_t)serviceLifePercent;
}

void MessageTypeCreateDeviceDiagnostic(messageTypeDiagnostic_t* deviceDiagn, const SettingDevice_t* setting)
{   
    deviceDiagn->hepa1 = setting->alarmError.hepa1Filter;
    deviceDiagn->hepa2 = setting->alarmError.hepa2Filter;
    deviceDiagn->preFilter = setting->alarmError.preFilter;

    uint32_t volt = UvLampGetMeanMiliVolt(UV_LAMP_1);
    deviceDiagn->ballast1Uv = volt;

    volt = UvLampGetMeanMiliVolt(UV_LAMP_2);
    deviceDiagn->ballast2Uv = volt;

    deviceDiagn->uv1Relay = setting->uvLamp1On;
    deviceDiagn->uv2Relay = setting->uvLamp2On;

    deviceDiagn->wifiOn = setting->restore.isWifiOn;
        
    int16_t count = 0 ;
    FanGetTachoRevolutionsPerSecond(&count);
    deviceDiagn->fanSpeed = count;
    
    deviceDiagn->fanLevel = 0;
    if(setting->restore.deviceStatus.isDeviceOn == true){
        deviceDiagn->fanLevel = setting->restore.deviceStatus.fanLevel + 1;
    }

    deviceDiagn->touchLock = setting->restore.touchLock;

    deviceDiagn->timerUv1 = TIMER_DRIVER_RAW_DATA_TO_HOUR(setting->restore.liveTime[TIMER_NAME_UV_LAMP_1]);
    deviceDiagn->timerUv2 = TIMER_DRIVER_RAW_DATA_TO_HOUR(setting->restore.liveTime[TIMER_NAME_UV_LAMP_2]);
    deviceDiagn->timerHepa = TIMER_DRIVER_RAW_DATA_TO_HOUR(setting->restore.liveTime[TIMER_NAME_HEPA]);
    deviceDiagn->timerTotal = TIMER_DRIVER_RAW_DATA_TO_HOUR(setting->restore.liveTime[TIMER_NAME_GLOBAL_ON]);
} 