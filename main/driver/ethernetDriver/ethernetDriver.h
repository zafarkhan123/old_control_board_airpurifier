/**
 * @file ethernet.h
 *
 * @brief ethernet driver
 *
 * @author matfio
 * @date 2021.11.17
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "enc28j60/enc28j60.h"

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize Ethernet driver
 *  @return return true if success
 */
bool ethernetDriverInit(void);

/** @brief Return information if additional pcb is palced
 *  @return return true if yes
 */
bool ethernetDriverIsAdditionalPcbConnected(void);

/** @brief Get ethernet status
 *  @return return eth_event_t event status
 */
eth_event_t ethernetDriverGetStatus(void);