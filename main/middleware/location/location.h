/**
 * @file location.h
 *
 * @brief Location middleware header file
 *
 * @author matfio
 * @date 2021.11.24
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

#define LOCATION_ADDRESS_NAME_STRING_LEN (256U)
#define LOCATION_ROOM_NAME_STRING_LEN (32U)

/*****************************************************************************
                       PUBLIC STRUCT
*****************************************************************************/

typedef struct
{
    char address[LOCATION_ADDRESS_NAME_STRING_LEN];
    char room[LOCATION_ROOM_NAME_STRING_LEN];
    float utcTimeHoursOffset; // UTC offset (in hours)
} __attribute__ ((packed)) Location_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize location struct
 *  @return true if success
 */
bool LocationInit(void);

/** @brief Load location from Non-volatile storage
 *  @return true if success
 */
bool LocationLoad(void);

/** @brief Printf location
 *  @param location [in] pointer to Location_t
 */
void LocationPrintf(Location_t* location);

/** @brief Get location struct
 *  @param location [out] pointer to Location_t
 *  @return return true if success, false mutex was not released
 */
bool LocationGet(Location_t* location);

/** @brief Set location struct
 *  @param location [in] pointer to Location_t
 *  @return return true if success, false mutex was not released
 */
bool LocationSet(Location_t* location);

/** @brief Save location to Non-volatile storage
 *  @return true if success
 */
bool LocationSave(void);

/** @brief Get utc time zone offset
 *  @return return utc offset
 */
float LocationGetUtcOffset(void);

/** @brief Set utc time zone offset
 *  @param offset  offset to save
 *  @return return true if success, false mutex was not released
 */
bool LocationSetUtcOffset(float offset);