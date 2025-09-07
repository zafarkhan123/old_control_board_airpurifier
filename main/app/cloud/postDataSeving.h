/**
 * @file postDataSeving.h
 *
 * @brief Post Data Seving middleware header file
 *
 * @author matfio
 * @date 2021.12.02
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "config.h"

#include "common/messageType.h"

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

/*****************************************************************************
                       PUBLIC STRUCTS
*****************************************************************************/

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Load unsend post data and initailize cirqular queue
 *  @return return true if success
 */
bool PostDataSevingInit(void);

/** @brief Write post data to nvs
 *  @param statusPost [in] pointer to messageTypeDeviceStatusHttpClient_t
 */
void PostDataSevingWriteNvs(messageTypeDeviceStatusHttpClient_t* statusPost);

/** @brief Get number of elements in cirqular queue
 *  @return return number
 */
uint16_t PostDataSevingReadSize(void);

/** @brief Read single element from cirqular queue
 *  @param statusPost [out] pointer to read struct messageTypeDeviceStatusHttpClient_t
 *  @return return true if success
 */
bool PostDataSevingRead(messageTypeDeviceStatusHttpClient_t* statusPost);

/** @brief Clear post data from nvs
 *  @return return true if success
 */
bool PostDataSevingClear(void);