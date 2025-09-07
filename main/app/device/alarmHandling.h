/**
 * @file Alarm Handling.h
 *
 * @brief Alarm handling header file
 *
 * @author matfio
 * @date 2021.11.03
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "setting.h"

#include "gpioExpanderDriver/gpioExpanderDriver.h"

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

typedef enum{
    ERROR_CODE_POWER_OFF = 1,
    ERROR_CODE_DATE_TIME_ERROR = 2,
    ERROR_CODE_PRE_FILTER_CICRUIT_OPEN = 3,
    ERROR_CODE_HEPA_1_FILTER_LIMIT_SWITCH = 4,
    ERROR_CODE_HEPA_2_FILTER_LIMIT_SWITCH = 5,
    ERROR_CODE_UV_1_POWER_CIRCUIT_FAULT = 6,
    ERROR_CODE_UV_2_POWER_CIRCUIT_FAULT = 7,
    ERROR_CODE_FAN_CIRCUIT_FAULT = 8,
    ERROR_CODE_FILTER_SERVICE_LIFE_EXCEEDED = 9,
    ERROR_CODE_UV_LAMPS_SERVICE_LIFE_EXCEEDED = 10,
    ERROR_CODE_INTERNAL_MEMORY_ERROR = 11,
    ERROR_CODE_UV_LAMPS_CONTROL_ERROR = 12,
}ErrorCode_t;

typedef enum{
    WARNING_CODE_POWER_BACK = 101,
    WARNING_CODE_HEPA_FILTER_CHANGE_REMAINDER = 125,
    WARNING_CODE_UV_LAMP_CHANGE_REMAINDER = 126,
}WarningCode_t;

/*****************************************************************************
                    PUBLIC STRUCTS / VARIABLES
*****************************************************************************/

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Check if error appear
 *  @param setting [in] pointer to SettingDevice_t struct
 *  @param expanderInput [in] pointer to GpioExpanderPinout_t struct
 *  @return return true if error
 */
bool AlarmHandlingErrorCheck(SettingDevice_t* setting, GpioExpanderPinout_t *expanderInput);

/** @brief Check if warning appear
 *  @param setting [in] pointer to SettingDevice_t struct
 *  @return return true if warning
 */
bool AlarmHandlingWarningCheck(SettingDevice_t* setting);

/** @brief Print warning, error information
 *  @param setting [in] pointer to SettingDevice_t struct
 */
void AlarmHandlingPrint(const SettingDevice_t* setting);

/** @brief If error appear do some action turn off fan and uv lamps
 *  @param setting [in] pointer to SettingDevice_t struct
 */
void AlarmHandlingManagement(SettingDevice_t* setting);

/** @brief Checking the wear of device components
 *  @param setting [out] pointer to SettingTimer_t
 *  @return return true when the device element is worn out
 */
bool AlarmHandlingTimersWornOutCheck(SettingDevice_t* setting);