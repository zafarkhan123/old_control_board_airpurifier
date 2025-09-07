/**
 * @file setting.h
 *
 * @brief setting middleware header file
 *
 * @author matfio
 * @date 2021.08.27
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "esp_wifi_types.h"

#include "ethernetDriver/ethernetDriver.h"

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

typedef enum{
    FAN_LEVEL_1 = 0,
    FAN_LEVEL_2,
    FAN_LEVEL_3,
    FAN_LEVEL_4,
    FAN_LEVEL_5,
    FAN_LEVEL_COUNT
}SettingFanLevel_t;

typedef enum{
    TIMER_NAME_HEPA = 0,
    TIMER_NAME_UV_LAMP_1,
    TIMER_NAME_UV_LAMP_2,
    TIMER_NAME_GLOBAL_ON,
    TIMER_NAME_COUNTER
}SettingTimerName_t;

typedef enum{
    DEVICE_MODE_MANUAL = 0,
    DEVICE_MODE_AUTOMATICAL
}SettingDeviceMode_t;

typedef enum{
    WIFI_STATUS_STA_START,                // wifi station start
    WIFI_STATUS_STA_STOP,                 // wifi station stop
    WIFI_STATUS_STA_CONNECTED,            // wifi station connected to AP
    WIFI_STATUS_STA_DISCONNECTED,         // ESP32 station disconnected from AP
}SettingWifiStatus_t;

typedef struct
{
    uint8_t isDetected      : 1;
    uint8_t memory          : 1;
    uint8_t rtc             : 1;
} __attribute__ ((packed)) SettingAlarmWarning_t;

typedef struct
{
    uint16_t isDetected         : 1;
    uint16_t uvLampBallast1     : 1;
    uint16_t uvLampBallast2     : 1;
    uint16_t preFilter          : 1;
    uint16_t hepa1Filter        : 1;
    uint16_t hepa2Filter        : 1;
    uint16_t fanSpeed           : 1;
    uint16_t stuckRelayUvLamp1  : 1;
    uint16_t stuckRelayUvLamp2  : 1;
} __attribute__ ((packed)) SettingAlarmError_t;

typedef struct
{
    uint8_t isWornOutDetected               : 1;
    uint8_t hepaFilterLifeTimeExpired       : 1;
    uint8_t hepaFilterReplacementReminder   : 1;
    uint8_t uvLamp1LifeTimeExpired          : 1;
    uint8_t uvLamp1ReplacementReminder      : 1;
    uint8_t uvLamp2LifeTimeExpired          : 1;
    uint8_t uvLamp2ReplacementReminder      : 1;
}__attribute__ ((packed))SettingTimersStatus_t;

typedef struct
{
    uint8_t isDeviceOn  : 1;
    uint8_t fanLevel    : 3;
    uint8_t isEkoOn     : 1;
    uint8_t unused      : 3;
} __attribute__ ((packed)) SettingDeviceStatust_t;

typedef struct
{
    uint32_t saveTimestamp;                     // save setting to NVS timestamp, used to checks if the battery from the external RCT has discharged
    SettingDeviceStatust_t deviceStatus;        // basic device settings
    uint64_t liveTime[TIMER_NAME_COUNTER];      // wear of components timer counter value, typed into the esp32 timers after power back
    uint8_t touchLock       : 1;                // lock touch pannel
    uint8_t deviceMode      : 1;                // SettingDeviceMode_t -> manual or auto  mode
    uint8_t isWifiOn        : 1;                // Wifi off / on flag
    uint8_t unused          : 5;                // for the future
    uint32_t unused2;
} __attribute__ ((packed)) SettingRestore_t;    // the settings needed to restore device status after a power back

typedef struct
{
    SettingRestore_t restore;                   // restores device state after power loss

    SettingWifiStatus_t wifiStatus;
    wifi_mode_t wifiMode;
    uint8_t tryConnectToNewAp           : 1;
    uint8_t isConnectNewAp              : 1;

    uint8_t uvLamp1On                   : 1;
    uint8_t uvLamp2On                   : 1;

    uint8_t ethernetPcbAdded            : 1;
    eth_event_t ethernetStatus;

    SettingAlarmWarning_t alarmWarning;
    SettingAlarmError_t alarmError;
    SettingTimersStatus_t timersStatus;

    uint8_t backFactorySetting          : 1;
    uint8_t deviceReset                 : 1;

    uint8_t newFirmwareVeryfication     : 1;
    
} __attribute__ ((packed)) SettingDevice_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize setting
 */
bool SettingInit(void);

/** @brief Get SettingDevice_t ,safe multi-thread
 *  @param setting [out] pointer to SettingDevice_t
 *  @return return true if success, false mutex was not released
 */
bool SettingGet(SettingDevice_t* setting);

/** @brief Set SettingDevice_t ,safe multi-thread
 *  @param setting [out] pointer to SettingDevice_t
 *  @return return true if success, false mutex was not released
 */
bool SettingSet(SettingDevice_t* setting);

/** @brief Load setting from Non-volatile storage
 */
bool SettingLoad(void);

/** @brief Save setting to Non-volatile storage
 */
bool SettingSave(void);

/** @brief Update SettingDeviceStatust_t ,safe multi-thread
 *  @param setting [in] pointer to SettingDevice_t
 *  @return return true if success, false mutex was not released
 */
bool SettingUpdateDeviceStatus(SettingDevice_t* setting);

/** @brief Update SettingDeviceStatust_t ,safe multi-thread
 *  @param setting [in] pointer to SettingTimer_t
 *  @return return true if success, false mutex was not released
 */
bool SettingUpdateTimers(SettingDevice_t* timer);

/** @brief Update device mode ,safe multi-thread
 *  @param setting [in] pointer to SettingTimer_t
 *  @return return true if success, false mutex was not released
 */
bool SettingUpdateDeviceMode(SettingDevice_t* setting);

/** @brief Lock or unlock touch screen ,safe multi-thread
 *  @param lock true - yes touch lock, false - screen unlock
 *  @return return true if success, false mutex was not released
 */
bool SettingUpdateTouchScreen(const bool lock);

/** @brief Is nvs memory error occurs
 *  @return true if error occurs
 */
bool SettingIsError(void);