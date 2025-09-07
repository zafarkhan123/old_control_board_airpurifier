/**
 * @file rtcDriver.h
 *
 * @brief Rtc driver header file
 *
 * @author matfio
 * @date 2021.11.05
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <time.h>

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

/*****************************************************************************
                       PUBLIC STRUCTS
*****************************************************************************/

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize rtc
 *  @return return true if success
 */
bool RtcDriverInit(void);

/** @brief Set rtc date and time
 *  @param time [in] pointer to struct tm
 *  @return return true if success
 */
bool RtcDriverSetDateTime(struct tm *time);

/** @brief Get rtc date and time
 *  @param time [out] pointer to struct tm
 *  @return return true if success
 */
bool RtcDriverGetDateTime(struct tm *time);

/** @brief Is rtc error
 *  @return true if error occurs
 */
bool RtcDriverIsError(void);