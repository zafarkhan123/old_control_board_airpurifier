/**
 * @file led.h
 *
 * @brief Rgb led middleware header file
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

/** @brief Initialize led
 *  @return return true if success
 */
bool LedInit(void);

/** @brief Changes the colors of the LEDs depending on the status of the device
 *  @param settingDevice pointer to global device setting
 *  @return return true if success
 */
bool LedChangeColor(const SettingDevice_t* settingDevice);

/** @brief Toggle wifi led when device is connecting to new AP
 *  @param settingDevice pointer to global device setting
 *  @return return true if success
 */
bool LedToggleWifi(const SettingDevice_t* settingDevice);

/** @brief Run lock touch screen sequence
 */
void LedLockSequenceStart(void);

/** @brief When factory reset start all led need to be blue to inform user
 *  @return return true if success
 */
bool LedResetFactoryInformation(void);