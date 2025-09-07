/**
 * @file touch.h
 *
 * @brief Touch middleware header file
 *
 * @author matfio
 * @date 2021.08.30
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "setting.h"

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

/*****************************************************************************
                       PUBLIC STRUCTS
*****************************************************************************/

typedef enum{
    TOUCH_BUTTON_PRESS_NO = 0,
    TOUCH_BUTTON_PRESS_SHORT,
    TOUCH_BUTTON_PRESS_LONG,
    TOUCH_BUTTON_PRESS_VERY_LONG
}TouchButtonPress_t;

typedef struct{
    TouchButtonPress_t status[CFG_TOUCH_BUTTON_NAME_COUNT];
} __attribute__ ((packed)) TouchButtons_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize touch
 *  @return return true if success
 */
bool TouchInit(void);

/** @brief Get button status. How long the button was pressed
 *  @param TouchButtons_t [out] pointer to TouchButtons_t status structure
 *  @return return true if button was pressed
 */
bool TouchButtonStatus(TouchButtons_t* buttons);

/** @brief Change the device setting according to which button is pressed
 *  @param settingDevice [out] pointer to SettingDevice_t
 *  @param buttons [in] pointer to TouchButtons_t
 *  @return return true if success
 */
bool TouchChangeDeviceSetting(SettingDevice_t* settingDevice, TouchButtons_t* buttons);