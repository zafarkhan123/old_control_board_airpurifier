/**
 * @file uvLampDriver.h
 *
 * @brief UV Lamp driver header file
 *
 * @author matfio
 * @date 2021.08.23
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

/*****************************************************************************
                       PUBLIC STRUCTS
*****************************************************************************/

typedef enum
{
    UV_LAMP_1 = 0,
    UV_LAMP_2,
    UV_LAMP_COUNT
}uvLampNumber_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Uv lamp driver initialization
 *  @return return true if success 
 */
bool UvLampDriverInit(void);

/** @brief On / off selected uv lamp
 *  @param lampNumber lamp number whose state we want to change
 *  @param level 0 - off, 1 - on
 *  @return return true if set correctly 
 */
bool UvLampDriverSetLevel(uvLampNumber_t lampNumber, uint32_t level);