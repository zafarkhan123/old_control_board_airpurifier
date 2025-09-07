/**
 * @file gpioIsrDriver.c
 *
 * @brief Gpio isr driver source file
 *
 * @dir gpioIsrDriver
 * @brief gpioIsrDriver driver folder
 *
 * @author matfio
 * @date 2021.08.23
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "config.h"

#include "driver/gpio.h"

#include "gpioIsrDriver.h"
#include "gpioExpanderDriver/gpioExpanderDriver.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define ESP_INTR_FLAG_DEFAULT (0)

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

/** @brief ISR handler function for the corresponding GPIO number
 *  @param arg parameter for ISR handler (in this solution the pin number)
 */

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    GpioExpanderDriverIrqChangeCallback(arg);
}

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool GpioIsrDriverInit(void)
{
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    gpio_isr_handler_add(CFG_GPIO_EXPANDER_INT_GPIO_PIN, gpio_isr_handler, (void*) CFG_GPIO_EXPANDER_INT_GPIO_PIN);
        
    return true;
}