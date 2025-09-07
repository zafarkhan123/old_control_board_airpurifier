/**
 * @file nvsDriver.h
 *
 * @brief Non-volatile storage driver header file
 *
 * @author matfio
 * @date 2021.09.07
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

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize Non-volatile storage driver
 *  @return return true if success
 */
bool NvsDriverInit(void);

/** @brief Set data (blob) value for given key
 *  @param key [in] key name
 *  @param outValue [in] the value to set
 *  @param valueLen [in] length of binary value to set
 *  @return return true if success
 */
bool NvsDriverSave(char *key, void *outValue, uint16_t valueLen);

/** @brief Get data (blob) value from given key
 *  @param key [in] key name
 *  @param inValue [out] pointer to the output value
 *  @param valueLen [out] length of binary value to get
 *  @return return true if success
 */
bool NvsDriverLoad(char *key, void *inValue, uint16_t* valueLen);