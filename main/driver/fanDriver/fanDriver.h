/**
 * @file fanDriver.h
 *
 * @brief Fan driver header file
 *
 * @author matfio
 * @date 2021.08.24
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize fan driver
 *  @return return true if success
 */
bool FanDriverInit(void);

/** @brief Set PWM duty
 *  @param duty range of duty setting is [0, (2**12) - 1]
 *  @return return true if success
 */
bool FanDriverSetDuty(uint32_t duty);

/** @brief Get tacho count from fan
 *  @return actual count values
 */
int16_t FanDriverGetTachoCount(void);