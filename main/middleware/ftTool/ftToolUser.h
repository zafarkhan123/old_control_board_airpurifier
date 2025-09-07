/**
 * @file ftToolUser.h
 *
 * @brief ft tool user middleware header file
 *
 * @author matfio
 * @date 2021.10.19
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/
/** @brief Read pwm fan level
 */
bool FtToolUserReadPwmFanLevel(uint8_t channel, void *dataPtr, uint32_t dataSize);

/** @brief Set pwm fan level
 */
bool FtToolUserWritePwmFanLevel(uint8_t channel, const void *dataPtr, uint32_t dataSize);

/** @brief Read off color component
 */
bool FtToolUserReadColorOffComponent(uint8_t channel, void *dataPtr, uint32_t dataSize);

/** @brief Write off color component
 */
bool FtToolUserWriteColorOffComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize);

/** @brief Read white color component
 */
bool FtToolUserReadColorWhiteComponent(uint8_t channel, void *dataPtr, uint32_t dataSize);

/** @brief Write white color component
 */
bool FtToolUserWriteColorWhiteComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize);

/** @brief Read red color component
 */
bool FtToolUserReadColorRedComponent(uint8_t channel, void *dataPtr, uint32_t dataSize);

/** @brief Write red color component
 */
bool FtToolUserWriteColorRedComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize);

/** @brief Read green color component
 */
bool FtToolUserReadColorGreenComponent(uint8_t channel, void *dataPtr, uint32_t dataSize);

/** @brief Write green color component
 */
bool FtToolUserWriteColorGreenComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize);

/** @brief Read blue color component
 */
bool FtToolUserReadColorBlueComponent(uint8_t channel, void *dataPtr, uint32_t dataSize);

/** @brief Write blue color component
 */
bool FtToolUserWriteColorBlueComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize);

/** @brief Read orange color component
 */
bool FtToolUserReadColorOrangeComponent(uint8_t channel, void *dataPtr, uint32_t dataSize);

/** @brief Write orange color component
 */
bool FtToolUserWriteColorOrangeComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize);

/** @brief Read tacho fan speed revolutions per second
 */
bool FtToolUserReadTachoSpeed(uint8_t channel, void *dataPtr, uint32_t dataSize);

/** @brief Read uv lamp ballast mili volt
 */
bool FtToolUserReadUvLampVoltage(uint8_t channel, void *dataPtr, uint32_t dataSize);