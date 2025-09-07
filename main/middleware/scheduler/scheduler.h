/**
 * @file scheduler.h
 *
 * @brief Scheduler middleware header file
 *
 * @author matfio
 * @date 2021.09.08
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

typedef enum{
    SCHEDULER_DAY_MONDAY = 0,
    SCHEDULER_DAY_TUESDAY,
    SCHEDULER_DAY_WEDNESDAY,
    SCHEDULER_DAY_THURSDAY,
    SCHEDULER_DAY_FRIDAY,
    SCHEDULER_DAY_SATURDAY,
    SCHEDULER_DAY_SUNDAY,
    SCHEDULER_DAY_COUNT
}SchedulerDay_t;

typedef enum{
    SCHEDULER_HOUR_0 = 0,
    SCHEDULER_HOUR_1,
    SCHEDULER_HOUR_2,
    SCHEDULER_HOUR_3,
    SCHEDULER_HOUR_4,
    SCHEDULER_HOUR_5,
    SCHEDULER_HOUR_6,
    SCHEDULER_HOUR_7,
    SCHEDULER_HOUR_8,
    SCHEDULER_HOUR_9,
    SCHEDULER_HOUR_10,
    SCHEDULER_HOUR_11,
    SCHEDULER_HOUR_12,
    SCHEDULER_HOUR_13,
    SCHEDULER_HOUR_14,
    SCHEDULER_HOUR_15,
    SCHEDULER_HOUR_16,
    SCHEDULER_HOUR_17,
    SCHEDULER_HOUR_18,
    SCHEDULER_HOUR_19,
    SCHEDULER_HOUR_20,
    SCHEDULER_HOUR_21,
    SCHEDULER_HOUR_22,
    SCHEDULER_HOUR_23,
    SCHEDULER_HOUR_COUNT
}SchedulerHour_t;

/*****************************************************************************
                       PUBLIC STRUCT
*****************************************************************************/

typedef struct
{
    SettingDeviceStatust_t hours[SCHEDULER_HOUR_COUNT];
} __attribute__ ((packed)) SchedulerofDay_t;

typedef struct
{
    SchedulerofDay_t days[SCHEDULER_DAY_COUNT];
} __attribute__ ((packed)) Scheduler_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize scheduler
 *  @return true if success
 */
bool SchedulerInit(void);

/** @brief Load scheduler from Non-volatile storage
 *  @return true if success
 */
bool SchedulerLoad(void);

/** @brief Save scheduler to Non-volatile storage
 *  @return true if success
 */
bool SchedulerSave(void);

/** @brief Get all scheduler
 *  @param scheduler [in] pointer to scheduler
 *  @return return true if success, false mutex was not released
 */
bool SchedulerGetAll(Scheduler_t* scheduler);

/** @brief Set status scheduler
 *  @param scheduler [in] pointer to scheduler
 *  @return return true if success, false mutex was not released
 */
bool SchedulerSetAll(Scheduler_t* scheduler);

/** @brief Get single day device status from week scheduler
 *  @param day selected day
 *  @param dayScheduler [out] pointer to day device status 
 *  @return return true if success, false mutex was not released
 */
bool SchedulerGetSingleDay(SchedulerDay_t day, SchedulerofDay_t* dayScheduler);

/** @brief Save day device status scheduler
 *  @param day selected day
 *  @param dayScheduler [in] pointer to day device status 
 *  @return return true if success, false mutex was not released
 */
bool SchedulerSetSingleDay(SchedulerDay_t day, SchedulerofDay_t* dayScheduler);

/** @brief Get single hour device status from selected day and hour
 *  @param day selected day
 *  @param hour selected hour
 *  @param hourScheduler [out] pointer to hour device status 
 *  @return return true if success, false mutex was not released
 */
bool SchedulerGetSingleHourOfDay(SchedulerDay_t day, SchedulerHour_t hour, SettingDeviceStatust_t* hourScheduler);

/** @brief Get single hour device status for actual day and hour
 *  @param deviceSetting [in] device setting to update
 *  @return return true if success, false mutex was not released
 */
bool SchedulerGetCurrentDeviceStatus(SettingDevice_t* deviceSetting);

/** @brief Checks if a scheduler reading is needed
 *  @param deviceSetting [in] to check device mode
 *  @return return true if success
 */
bool SchedulerIsDeviceStatusUpdateNeeded(SettingDevice_t* deviceSetting);

/** @brief Get day name string
 *  @param day [in] day enum SchedulerDay_t
 *  @return string pointer
 */
const char* SchedulerGetStringDayName(SchedulerDay_t day);

/** @brief Printf scheduler
 *  @param scheduler [in] pointer to Scheduler_t
 */
void SchedulerPrintf(Scheduler_t* scheduler);