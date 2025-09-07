/**
 * @file messageType.h
 *
 * @brief message type header file
 *
 * @author matfio
 * @date 2021.09.01
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "setting.h"

#include "scheduler/scheduler.h"

/*****************************************************************************
                          PUBLIC DEFINES / MACROS
 *****************************************************************************/

#define MESSAGE_TYPE_MAX_DEVICE_INFO_JSON_LENGTH        (384U)
#define MESSAGE_TYPE_MAX_DEVICE_STATUS_JSON_LENGTH      (512U)
#define MESSAGE_TYPE_MAX_DEVICE_LOCATION_JSON_LENGTH    (512U)
#define MESSAGE_TYPE_MAX_DEVICE_SCHEDULER_JSON_LENGTH   (1800U)
#define MESSAGE_TYPE_MAX_DEVICE_MODE_JSON_LENGTH        (256U)
#define MESSAGE_TYPE_MAX_WIFI_SETTING_JSON_LENGTH       (6U * 1024U)
#define MESSAGE_TYPE_MAX_DEVICE_TIME_JSON_LENGTH        (256U)
#define MESSAGE_TYPE_MAX_CLEAR_COUNTER_JSON_LENGTH      (256U)
#define MESSAGE_TYPE_MAX_DEVICE_DIAGNOSTIC_JSON_LENGTH  (512U)
#define MESSAGE_TYPE_MAX_DEVICE_AUTH_JSON_LENGTH        (256U)

#define MESSAGE_TYPE_ALARM_CODE_ARRAY_LEN (16U)

/*****************************************************************************
                       PUBLIC STRUCTS
*****************************************************************************/

typedef struct
{
    uint8_t Switch;
    SettingFanLevel_t fan;
    uint16_t lamp;
    uint16_t hepa;
    char* sw_version;
    char* compile_date;
    char* compile_time;
    uint8_t automatical;
    uint8_t wifiConnect;
    uint8_t ecoOn           : 1;
    uint8_t lockOn          : 1;
    uint32_t timestamp;
} __attribute__ ((packed)) messageTypeDeviceInfo_t;

typedef struct
{
    uint8_t Switch          : 1;
    SettingFanLevel_t fan;
    SettingDeviceMode_t automatical;
    uint8_t wifiConnect     : 1;
    uint8_t ecoOn           : 1;
    uint8_t lockOn          : 1;

} __attribute__ ((packed)) messageTypeDeviceMode_t;

typedef struct
{
    uint8_t setting         : 3;
    uint8_t isEco           : 1;
    uint8_t                 : 4;
} __attribute__ ((packed)) messageTypeDeviceSetting_t;

typedef struct
{
    uint32_t timestamp;
    messageTypeDeviceSetting_t deviceSetting[SCHEDULER_DAY_COUNT][SCHEDULER_HOUR_COUNT];
} __attribute__ ((packed)) messageTypeScheduler_t;

typedef struct
{
    char* hwVersion;
    char* swVersion;
} __attribute__ ((packed)) messageTypeDeviceInfoHttpClient_t;

typedef struct
{
    uint32_t timestamp;
    uint16_t mode           : 1;
    uint32_t totalOn;
    uint32_t timUv1;
    uint32_t timUv2;
    uint32_t timHepa;
    uint32_t rtc;
    uint16_t fanLevel       : 3;
    uint16_t isEco          : 1;
    uint16_t alarmCodeIdx;
    uint8_t alarmCode[MESSAGE_TYPE_ALARM_CODE_ARRAY_LEN];
    uint16_t ethernetOn     : 1;
    uint16_t touchLock      : 1;
    uint16_t wifiOn         : 1;
    uint16_t deviceReset    : 1;
    uint16_t resetReason    : 3;
} __attribute__ ((packed)) messageTypeDeviceStatusHttpClient_t;

typedef struct
{
    uint32_t timestamp;
    uint16_t mode           : 1;
    uint16_t fanLevel       : 3;
    uint16_t isEco          : 1;
    uint16_t touchLock      : 1;
} __attribute__ ((packed)) messageTypeDeviceModeHttpClient_t;

typedef struct
{
    uint16_t deviceReset        : 1;
    uint16_t uv1TimerReload     : 1;
    uint16_t uv2TimerReload     : 1;
    uint16_t hepaTimerReload    : 1;
    uint16_t scheduleReset      : 1;
    uint16_t rtcTimeIsSet       : 1;
    uint32_t rtcTime;
    uint16_t hepaLivespanIsSet  : 1;
    uint16_t hepaLivespan;
    uint16_t hepaWarningIsSet   : 1;
    uint16_t hepaWarning;
    uint16_t uvLivespanIsSet    : 1;
    uint16_t uvLivespan;
    uint16_t uvWarningIsSet     : 1;
    uint16_t uvWarning;
    uint16_t utcTimeOffsetIsSet : 1;
    float utcTimeOffset;
} __attribute__ ((packed)) messageTypeDeviceServiceHttpClient_t;

typedef struct
{
    uint8_t hepaCounter         : 1;
    uint8_t uvLamp1Counter      : 1;
    uint8_t uvLamp2Counter      : 1;
} __attribute__ ((packed)) messageTypeClearCounter_t;

typedef struct
{
    uint8_t hepa1               : 1;
    uint8_t hepa2               : 1;
    uint8_t preFilter           : 1;
    uint32_t ballast1Uv;
    uint32_t ballast2Uv;
    uint8_t uv1Relay            : 1;
    uint8_t uv2Relay            : 1;    
    uint8_t wifiOn              : 1;
    int16_t fanSpeed;
    uint8_t fanLevel;
    uint8_t touchLock           : 1;
    uint32_t timerUv1;
    uint32_t timerUv2;
    uint32_t timerHepa;
    uint32_t timerTotal;
} __attribute__ ((packed)) messageTypeDiagnostic_t;


typedef enum
{
    MESSAGE_TYPE_AUTH_TYPE_FAIL = 0,
    MESSAGE_TYPE_AUTH_TYPE_SERVICE,
    MESSAGE_TYPE_AUTH_TYPE_DIAGNOSTIC
}messageTypeDeviceAuthType_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Create messageTypeDeviceInfo_t from the SettingDevice_t
 *  @param deviceInfo [out] pointer to messageTypeDeviceInfo_t
 *  @param setting [in] pointer to SettingDevice_t
 */
void MessageTypeCreateDeviceInfo(messageTypeDeviceInfo_t* deviceInfo, const SettingDevice_t* setting);

/** @brief Create SettingDevice_t from messageTypeDeviceMode_t
 *  @param mode [in] pointer to messageTypeDeviceMode_t
 *  @param setting [out] pointer to SettingDevice_t
 */
void MessageTypeCreateSettingDevice(messageTypeDeviceMode_t* mode, SettingDevice_t* setting);

/** @brief Create Scheduler_t from messageTypeScheduler_t
 *  @param message [in] pointer to messageTypeScheduler_t
 *  @param scheduler [out] pointer to Scheduler_t
 */
void MessageTypeCreateScheduler(const messageTypeScheduler_t* message, Scheduler_t* scheduler);

/** @brief Create messageTypeScheduler_t from Scheduler_t
 *  @param message [out] pointer to messageTypeScheduler_t
 *  @param scheduler [in] pointer to Scheduler_t
 */
void MessageTypeCreateMessageTypeScheduler(messageTypeScheduler_t* message, const Scheduler_t* scheduler);

/** @brief Create messageTypeDeviceInfoHttpClient_t type
 *  @param deviceInfo [out] pointer to messageTypeDeviceInfoHttpClient_t
 */
void MessageTypeCreateDeviceInfoHttpClient(messageTypeDeviceInfoHttpClient_t* deviceInfo);

/** @brief Create messageTypeDeviceStatusHttpClient_t from SettingDevice_t
 *  @param deviceStatus [out] pointer to messageTypeDeviceStatusHttpClient_t
 *  @param setting [in] pointer to SettingDevice_t
 */
void MessageTypeCreateDeviceStatusHttpClient(messageTypeDeviceStatusHttpClient_t* deviceStatus, const SettingDevice_t* setting);

/** @brief Create SettingDevice_t from messageTypeDeviceModeHttpClient_t
 *  @param deviceMode [in] pointer to messageTypeDeviceModeHttpClient_t
 *  @param setting [out] pointer to SettingDevice_t
 */
void MessageTypeCreateSettingFromDeviceModeHttpClient(const messageTypeDeviceModeHttpClient_t* deviceMode, SettingDevice_t* setting);

/** @brief Create messageTypeDeviceModeHttpClient_t from SettingDevice_t
 *  @param deviceMode [out] pointer to messageTypeDeviceModeHttpClient_t
 *  @param setting [in] pointer to SettingDevice_t
 */
void MessageTypeCreateDeviceModeHttpClientFromSetting(messageTypeDeviceModeHttpClient_t* deviceMode, const SettingDevice_t* setting);

/** @brief Get current hepa filter live time in percent
 *  @param actualLiveTime the current state of the counter
 *  @return life time in precent
 */
uint16_t MessageTypeGetHepaLifeTimeInPercent(uint64_t actualLiveTime);

/** @brief Get current uv lamp live time in percent
 *  @param actualLiveTime the current state of the counter
 *  @return life time in precent
 */
uint16_t MessageTypeGetUvLampLifeTimeInPercent(uint64_t actualLiveTime);

/** @brief Create messageTypeDiagnostic_t from SettingDevice_t
 *  @param deviceDiagn [out] pointer to messageTypeDiagnostic_t
 *  @param setting [in] pointer to SettingDevice_t
 */
void MessageTypeCreateDeviceDiagnostic(messageTypeDiagnostic_t* deviceDiagn, const SettingDevice_t* setting);
