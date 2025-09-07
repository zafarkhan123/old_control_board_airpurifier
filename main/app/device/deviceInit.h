/**
 * @file deviceInit.h
 *
 * @brief Device Initialization header file
 *
 * @author matfio
 * @date 2021.08.30
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
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize and run device tasks
 */
void DeviceInit(void);

/** @brief Initialize common i2c periphery used by rtc, touch and expander gpio
 *  @return return true if success
 */
bool DeviceInitCommonI2cInit(void);

/** @brief Initialize common SPI periphery used by ethernet and falsh
 *  @return return true if success
 */
bool DeviceInitCommonSpiInit(void);

/** @brief Read data from Nvs
 *  @return return true if success
 */
bool DeviceInitReadDataFromNvs(void);