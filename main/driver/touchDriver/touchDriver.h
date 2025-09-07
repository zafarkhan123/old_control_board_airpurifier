/**
 * @file touchDriver.h
 *
 * @brief Touch driver header file
 *
 * @author matfio
 * @date 2021.08.25
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include "config.h"

#include <stdint.h>
#include <stdbool.h>

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

#define TOUCH_CAP1293_PRODUCT_ID (0x6F)

typedef enum{
    TOUCH_POWER_STATE_ACTICE = 0,
    TOUCH_POWER_STATE_STANBY,
    TOUCH_POWER_STATE_COMBO,
    TOUCH_POWER_STATE_DSLEEP,
}TouchDriverPowerState_t;

typedef enum{
    TOUCH_INPUT_STATUS_NOTHING_DETECTED = 0,
    TOUCH_INPUT_STATUS_ERROR = -1,
    TOUCH_INPUT_STATUS_CHANGES_DETECTED = 1,
}TouchDriverInputStatus_t;

/*****************************************************************************
                       PUBLIC STRUCTS
*****************************************************************************/

typedef struct 
{
    bool isPressNow[CFG_TOUCH_BUTTON_NAME_COUNT];
} __attribute__ ((packed)) TouchDriverButtonStatus_t;

typedef struct
{
    uint8_t productId;
    uint8_t manufacturedId;
    uint8_t revision;
} __attribute__ ((packed)) TouchDriverInfo_t;


/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize touch driver
 *  @return return true if success
 */
bool TouchDriverInit(void);

/** @brief Get touch sensor information
 *  @param deviceInfo [out] pointer to TouchDriverInfo_t device info structure
 *  @return return TouchDriverInfo_t struct
 */
bool TouchDriverGetDeviceInfo(TouchDriverInfo_t* deviceInfo);

/** @brief Checks the state of the buttons
 *  @param buttonStatus [out] pointer to button input status
 *  @return return 0 - no touch detected, 1 - touch detected, -1 - error
 */
TouchDriverInputStatus_t TouchDriverIsButtonTouched(TouchDriverButtonStatus_t* buttonStatus);

/** @brief  Check if ALERT Pin is set
 *  @return return true if set
 */
bool TouchDriverIsAlertSet(void);

/** @brief Set touch device power state
 *  @param powerState [in] selected power state
 *  @return return true if success
 */
bool TouchDriverSetPowerState(TouchDriverPowerState_t powerState);