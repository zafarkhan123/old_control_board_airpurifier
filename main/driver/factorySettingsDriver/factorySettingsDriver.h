/**
 * @file factorySettingsDriver
 *
 * @brief Non-volatile storage driver header file for factory settings
 *
 * @author matfio
 * @date 2021.10.07
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "setting.h"
#include "scheduler/scheduler.h"

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

typedef enum{
    FACTORY_SETTING_SERVICE_HEPA_LIFETIME_HOURS = 0,
    FACTORY_SETTING_SERVICE_HEPA_WARNING_HOURS,
    FACTORY_SETTING_SERVICE_UV_LIFETIME_HOURS,
    FACTORY_SETTING_SERVICE_UV_WARNING_HOURS,
    FACTORY_SETTING_SERVICE_UV_OFF_MIN_MILIVOLT,
    FACTORY_SETTING_SERVICE_UV_OFF_MAX_MILIVOLT,
    FACTORY_SETTING_SERVICE_UV_ON_MIN_MILIVOLT,
    FACTORY_SETTING_SERVICE_UV_ON_MAX_MILIVOLT,
    FACTORY_SETTING_SERVICE_LOGO_LED_COLOR,
    FACTORY_SETTING_SERVICE_CLOUD_PORT,
    FACTORY_SETTING_SERVICE_COUNT,
}FactorySettingServiceParam_t;

/*****************************************************************************
                       PUBLIC STRUCTS
*****************************************************************************/

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize factory partition
 *  @return true if success
 */
bool FactorySettingsDriverInit(void);

/** @brief Get device name string read from factory partition
 *  @return pointer to string
 */
char* FactorySettingsGetDevceName(void);

/** @brief Set new device name string to factory partition
 *  @param newDeviceName new device name
 *  @return true if success
 */
bool FactorySettingsSetDevceName(char* newDeviceName);

/** @brief Get device type string read from factory partition
 *  @return pointer to string
 */
char* FactorySettingsGetDevceType(void);

/** @brief Get hardware version string read from factory partition
 *  @return pointer to string
 */
char* FactorySettingsGetHardwareVersion(void);

/** @brief Get location string read from factory partition
 *  @return pointer to string
 */
char* FactorySettingsGetLocation(void);

/** @brief Get pwm value to set fan speed read from factory partition
 *  @param levelNumber level number
 *  @param pwmValue [out] pointer to read data
 *  @return true if success
 */
bool FactorySettingsGetPwmFanLevel(SettingFanLevel_t levelNumber, uint32_t* pwmValue);

/** @brief Get default factory  scheduler
 *  @param scheduler [out] pointer to Scheduler_t
 *  @return true if success
 */
bool FactorySettingsGetScheduler(Scheduler_t* scheduler);

/** @brief Get service setting
 *  @param serviceParam service value
 *  @param pwmValue [out] pointer to read data
 *  @return true if success
 */
bool FactorySettingsGetServiceParam(FactorySettingServiceParam_t serviceParam, uint32_t* serviceValue);

/** @brief Update new service setting
 *  @param serviceParam service value
 *  @param pwmValue [out] pointer to read data
 *  @return true if success
 */
bool FactorySettingsUpdateServiceParam(FactorySettingServiceParam_t serviceParam, uint32_t* serviceValue);

/** @brief Get service password string read from factory partition
 *  @return pointer to string
 */
char* FactorySettingsGetServicePassword(void);

/** @brief Get diagnostic password string read from factory partition
 *  @return pointer to string
 */
char* FactorySettingsGetDiagnosticPassword(void);

/** @brief Get cloud host name from factory partition
 *  @return pointer to string
 */
char* FactorySettingsGetCloudHostName(void);

/** @brief Get scope ID from factory partition
 *  @return pointer to string
 */
char* FactorySettingsGetScopeIdName(void);

/** @brief Get client_cert from factory partition
 *  @return pointer to string
 */
char* FactorySettingsGetClientCert(void);

/** @brief Get client_key from factory partition
 *  @return pointer to string
 */
char* FactorySettingsGetClientKey(void);