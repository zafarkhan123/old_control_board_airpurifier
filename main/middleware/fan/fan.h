/**
 * @file fan.h
 *
 * @brief Fan middleware header file
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

#include "setting.h"

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

typedef enum{
    FAN_TACHO_ERROR = -1,
    FAN_TACHO_DEVICE_OFF = 0,
    FAN_TACHO_STARTS,
    FAN_TACHO_WORKS,
}FanTachoState_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize fan
 *  @return return true if success
 */
bool FanInit(void);

/** @brief Set fan level according device setting
 *  @param settingDevice pointer to global device setting
 *  @return return true if success
 */
bool FanLevelChange(const SettingDevice_t* settingDevice);

/** @brief Get pwm value for selected fan level (ft tool use it)
 *  @param level fan level number
 *  @return return actual pwm value
 */
uint32_t FanGetActualPwmFanLevel(SettingFanLevel_t level);

/** @brief Set new pwm value for selected fan level (ft tool use it)
 *  @param level fan level number
 *  @param newPwmValue new pwm value
 */
void FanSetNewPwmFanLevel(SettingFanLevel_t level, uint32_t newPwmValue);

/** @brief Get tacho revolutions per second
 *  @param revolutions [out] pointer to result
 *  @return tacho status
 */
FanTachoState_t FanGetTachoRevolutionsPerSecond(int16_t* revolutions);