/**
 * @file uvLamp.h
 *
 * @brief UV LAmpp middleware header file
 *
 * @author matfio
 * @date 2021.11.02
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "uvLampDriver/uvLampDriver.h"

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

typedef enum
{
    UV_LAMP_STATUS_UNKNOWN = 0,
    UV_LAMP_STATUS_OFF,
    UV_LAMP_STATUS_ON,
    UV_LAMP_STATUS_ERROR,
}uvLampStatus_t;

/*****************************************************************************
                       PUBLIC STRUCTS
*****************************************************************************/

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize gpio, start uv lamp measurment timer,...
 *  @return return true if success
 */
bool UvLampInit(void);

/** @brief Manage uv lamp and timers
 *  @param setting [in] pointer to SettingDevice_t
 *  @return return true if success
 */
bool UvLampManagement(SettingDevice_t* setting);

/** @brief Execute on/off lamp with delay
 *  @param setting [in] pointer to SettingDevice_t
 *  @return return true if success
 */
bool UvLampExecute(SettingDevice_t* setting);

/** @brief Get actual uv lamp ballast status
 *  @param uvLampNumber_t ul lamp number
 *  @return return true if success
 */
uvLampStatus_t UvLampDriverGetUvLampStatus(uvLampNumber_t lampNumber);

/** @brief Emergency off uv lamp
 */
void UvLampEmergencyOff(void);

/** @brief Get meal milivolt from ballast
 *  @param uvLampNumber_t ul lamp number
 *  @return return true if success
 */
uint32_t UvLampGetMeanMiliVolt(uvLampNumber_t lampNumber);