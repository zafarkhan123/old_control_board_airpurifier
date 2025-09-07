/**
 * @file gpioExpanderDriver.h
 *
 * @brief Gpio Expander driver header file
 *
 * @author matfio
 * @date 2021.11.15
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
    uint8_t b0 : 1;
    uint8_t b1 : 1;
    uint8_t b2 : 1;
    uint8_t b3 : 1;
    uint8_t b4 : 1;
    uint8_t b5 : 1;
    uint8_t b6 : 1;
    uint8_t b7 : 1;
}__attribute__ ((packed)) GpioExpanderPca9534BitsReg_t;

typedef struct
{
    uint8_t wifiSwitch      : 1;
    uint8_t limitSwitch3    : 1; // preFilter
    uint8_t limitSwitch2    : 1; // hepa2
    uint8_t limitSwitch1    : 1; // hepa1
    uint8_t ledEnable       : 1;
    uint8_t buzzer          : 1;
    uint8_t nc1             : 1; // NC
    uint8_t nc2             : 1; // NC
}__attribute__ ((packed)) GpioExpanderPinout_t;

/*****************************************************************************
                       PUBLIC STRUCTS
*****************************************************************************/

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Expander gpio init
 *  @return return true if success
 */
bool GpioExpanderDriverInit(void);

/** @brief Off buzzer
 *  @return return true if success
 */
bool GpioExpanderDriverBuzzerOff(void);

/** @brief On buzzer
 *  @return return true if success
 */
bool GpioExpanderDriverBuzzerOn(void);

/** @brief Off leds
 *  @return return true if success
 */
bool GpioExpanderDriverLedOff(void);

/** @brief Check is buzzer on
 *  @return return true if on
 */
bool GpioExpanderDriverIsBuzzerOn(void);

/** @brief On leds
 *  @return return true if success
 */
bool GpioExpanderDriverLedOn(void);

/** @brief Set output port value
 *  @param outputPort output bX -> 0 - pin set to low value, 1 - pin set to high value
 *  @return return true if success
 */
bool GpioExpanderDriverSetOutputPort(GpioExpanderPinout_t outputPort);

/** @brief Get input port value
 *  @param inputPort input port vale
 *  @return return true if success
 */
bool GpioExpanderDriverGetInputPort(GpioExpanderPinout_t *inputPort);

/** @brief  Check if Interrupt Pin is set
 *  @return return true if set
 */
bool GpioExpanderDriverIsInterruptSet(void);

/** @brief Callback function executed when input port change state in expander
*/
void GpioExpanderDriverIrqChangeCallback(void* arg);

/** @brief Clear expandeer irq
*/
void GpioExpanderDriverClearIrq(void);

/** @brief  Print input status
 *  @param inputPort input port vale
 */
void GpioExpanderDriverPrintInputStatus(GpioExpanderPinout_t inputPort);