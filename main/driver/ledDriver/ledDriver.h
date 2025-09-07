/**
 * @file ledDriver.h
 *
 * @brief Rgb led driver header file
 *
 * @author matfio
 * @date 2021.08.20
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "led_strip/led_strip.h"

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

typedef enum{
    LED_NAME_PWR = 0,
    LED_NAME_LOCK,
    LED_NAME_LOGO_OPTIONAL,
    LED_NAME_LOGO,
    LED_NAME_FAN_SPEED_INCREASE,
    LED_NAME_FAN_STATUS,
    LED_NAME_FAN_SPEED_DECREASE,
    LED_NAME_FAN_SPEED_LEVEL_1,
    LED_NAME_FAN_SPEED_LEVEL_2,
    LED_NAME_FAN_SPEED_LEVEL_3,
    LED_NAME_FAN_SPEED_LEVEL_4,
    LED_NAME_FAN_SPEED_LEVEL_5,
    LED_NAME_ALARM,
    LED_NAME_WIFI_STATUS,
    LED_NAME_HEPA_STATUS,
    LED_NAME_UV_STATUS,
    LED_NAME_COUNT
}LedDriverName_t;

typedef enum{
    LED_COLOR_OFF = 0,
    LED_COLOR_WHITE,
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
    LED_COLOR_ORANGE,
    LED_COLOR_LOGO, // user-defined in factory settings
    LED_COLOR_COUNT
}LedDriverColor_t;

/*****************************************************************************
                    PUBLIC STRUCTS / VARIABLES
*****************************************************************************/

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize led driver
 *  @return return true if success
 */
bool LedDriverInit(void);

/** @brief Deinitialize led driver
 *  @return return true if success
 */
bool LedDriverDeinit(void);

/** @brief Set color of selected single LED. This function does not actually change colors of the LEDs. Call LedDriverSetColor() to send.
 *  @param ledName name of the diode whose color we want to change
 *  @param color new color
 *  @return return true if success
 */
bool LedDriverSetColor(LedDriverName_t ledName, LedDriverColor_t color);

/** @brief Change color in led bar. 
 *  @return return true - success, false - if associated RMT channel is busy
 */
bool LedDriverChangeColor(void);

/** @brief Get actual selected color components (ft tool use it)
 *  @param colorName the color components we want to get
 *  @return return RGB components
 */
rgb_t LedDriverGetColorComponents(LedDriverColor_t colorName);

/** @brief Set new color components (ft tool use it)
 *  @param colorName the color components we want to set
 *  @param newColorComp new RGB components to set
 */
void LedDriverSetColorComponents(LedDriverColor_t colorName, rgb_t newColorComp);