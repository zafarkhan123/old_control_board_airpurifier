/**
 * @file timerDriver.h
 *
 * @brief Timer driver header file
 *
 * @author matfio
 * @date 2021.09.02
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
// raw data in timer counter 1 bit = 0.1 [mS]
#define TIMER_DRIVER_RAW_COUNTER_TO_SECOND_MULTIPLER (10ULL * 1000ULL)

// Convert value in hours to timer counter raw date
#define TIMER_DRIVER_HOUR_TO_RAW_DATA(H) ((H) * (TIMER_DRIVER_RAW_COUNTER_TO_SECOND_MULTIPLER) * (60ULL) * (60ULL))

// Convert timer counter raw date to value in hours
#define TIMER_DRIVER_RAW_DATA_TO_HOUR(R) ((R) / ((TIMER_DRIVER_RAW_COUNTER_TO_SECOND_MULTIPLER) * (60ULL) * (60ULL)))

// Convert timer counter raw date to value in seconds
#define TIMER_DRIVER_RAW_DATA_TO_SECOND(R) ((R) / (TIMER_DRIVER_RAW_COUNTER_TO_SECOND_MULTIPLER))

/*****************************************************************************
                       PUBLIC STRUCTS
*****************************************************************************/

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Timer driver initialization
 *  @return return true if success 
 */
bool TimerDriverInit(void);

/** @brief Set timers counter register to start value
 *  @param setting pointer to SettingDevice_t
 *  @return return true if success
 */
bool TimerDriverSetTimers(const SettingDevice_t* setting);

/** @brief Pause selected timer
 *  @param timer selected timer
 *  @return return true if success
 */
bool TimerDriverPause(SettingTimerName_t timer);

/** @brief Start selected timer
 *  @param timer selected timer
 *  @return return true if success
 */
bool TimerDriverStart(SettingTimerName_t timer);

/** @brief Get counter raw date from timer (1 bit = 0.1 [mS])
 *  @param timer [in] selected timer
 *  @param counter [out] raw data
 *  @return return true if success
 */
bool TimerDriverGetCounter(SettingTimerName_t timer, uint64_t* counter);

/** @brief Clear counter
 *  @param timer [in] selected timer
 */
void TimerDriverClearCounter(SettingTimerName_t timer);

/** @brief Get counter second date from timer
 *  @param timer [in] selected timer
 *  @param counter [out] counter in seconds
 *  @return return true if success
 */
bool TimerDriverGetCounterSec(SettingTimerName_t timer, uint64_t* counter);

/** @brief Update SettingTimer_t structure
 *  @param setting [out] pointer to SettingTimer_t
 *  @return return true if success
 */
bool TimerDriverUpdateTimerSetting(SettingDevice_t* setting);