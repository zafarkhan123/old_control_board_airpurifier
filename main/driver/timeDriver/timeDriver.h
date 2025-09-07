/**
 * @file timeDriver.h
 *
 * @brief time driver header file
 *
 * @author matfio
 * @date 2021.08.31
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <sys/time.h>

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Get UTC unix time
 *  @return return unix time
 */
uint32_t TimeDriverGetUTCUnixTime(void);

/** @brief Get local unix time
 *  @return return unix time
 */
uint32_t TimeDriverGetLocalUnixTime(void);

/** @brief Get UTC time
 *  @return return tm struct time
 */
struct tm* TimeDriverGetUTCTime(void);

/** @brief Get local time
 *  @return return tm struct time
 */
struct tm* TimeDriverGetLocalTime(void);

/** @brief Get time in MS  since boot
 *  @return number of MS since underlying timer has been started 
 */
int64_t TimeDriverGetSystemTickMs(void);

/** @brief Check if time elapsed
 *  @param startTime start time
 *  @param deltaMsTime delta time in ms
 *  @return true time has passed
 */
bool TimeDriverHasTimeElapsed(int64_t startTime, uint32_t deltaMsTime);

/** @brief Set local time in esp32 devices
 */
void TimeDriverSetEspTime(struct tm* newTime);

/** @brief Set local time using unix in esp32 devices
 */
void TimeDriverSetEspTimeByUnixTime(uint32_t newTime);

/** @brief Get local time string
 *  @return return pointer to string
 */
char* TimeDriverGetLocalTimeStr(void);