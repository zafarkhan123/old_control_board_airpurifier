/**
 * @file iotHubClient.h
 *
 * @brief iotHubClient header file
 *
 * @author matfio
 * @date 2022.03.04
 * @version v1.0
 *
 * @copyright 2022 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/*****************************************************************************
                     PUBLIC STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

typedef struct{
    uint16_t isConnectedLeastOnce : 1;
    uint16_t : 15;
    uint32_t lastConnection;
} __attribute__ ((packed)) iotHubClientStatus_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Read data from Non-volatile storage and initialize
 *  @return true if success
 */
bool IotHubClientInit(void);

/** @brief Get IoT Hub setting struct
 *  @param setting [out] pointer to iotHubClientStatus_t
 *  @return return true if success, false mutex was not released
 */
bool IotHubClientGetSetting(iotHubClientStatus_t* setting);

/** @brief Set IoT Hub setting struct
 *  @param setting [in] pointer to iotHubClientStatus_t
 *  @return return true if success, false mutex was not released
 */
bool IotHubClientSetSetting(iotHubClientStatus_t* setting);

/** @brief Save IoT Hub setting to Non-volatile storage
 *  @return true if success
 */
bool IotHubClientSettingSave(void);

/** @brief IoT Hub client main loop
 * */
void IotHubClientMainLoop(void *argument);