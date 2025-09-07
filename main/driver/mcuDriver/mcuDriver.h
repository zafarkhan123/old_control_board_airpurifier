/**
 * @file mcuDriver.h
 *
 * @brief MCU driver
 *
 * @author matfio
 * @date 2021.10.28
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

typedef struct
{
    uint8_t part[6];
}mcuDriverMacAddress_t;


/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Restart esp32 device
 */
void McuDriverDeviceRestart(void);


/** @brief Restart esp32 device in safe way. 
 *  Save device setting, scheduler, timers, ...
 */
void McuDriverDeviceSafeRestart(void);

/** @brief Get wifi mac address
 */
mcuDriverMacAddress_t McuDriverGetWifiMac(void);

/** @brief Get wifi mac address string
 */
char* McuDriverGetWifiMacStr(void);

/** @brief Get device id
 */
uint64_t McuDriverGetDeviceId(void);