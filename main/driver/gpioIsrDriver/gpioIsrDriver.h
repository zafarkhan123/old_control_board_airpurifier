/**
 * @file gpioIsrDriver.h
 *
 * @brief Gpio isr driver header file
 *
 * @author matfio
 * @date 2021.08.23
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

/** @brief Add ISR handler for the corresponding GPIO pins.
 */
bool GpioIsrDriverInit(void);