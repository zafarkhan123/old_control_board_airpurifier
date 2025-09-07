/**
 * @file externalFlashDriver.h
 *
 * @brief external flash driver
 *
 * @author matfio
 * @date 2021.12.31
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

/** @brief Initialize external flash driver
 *  @return return true if success
 */
bool ExternalFlashDriverInit(void);

// Always timeout crashes , to be corrected if ever needed
/** @brief Erase external flash chip 
 *  @return return true if success
 */
bool ExternalFlashEraseChip(void);

/** @brief Mount file system
 *  @return return true if success
 */
bool ExternalFlashMountFs(void);

/** @brief Test with open reading and writing to a file
 *  @return return true if success
 */
bool ExternalFlashFileTest(void);
