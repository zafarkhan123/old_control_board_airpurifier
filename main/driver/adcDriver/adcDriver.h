/**
 * @file adcDriver.h
 *
 * @brief Adc driver header file
 *
 * @author matfio
 * @date 2021.08.24
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

typedef enum{
    ADC_DRIVER_CHANNEL_UV_1 = 0,
    ADC_DRIVER_CHANNEL_UV_2,
    ADC_DRIVER_CHANNEL_COUNT,
}AdcDriverChannel_t;

/*****************************************************************************
                       PUBLIC STRUCTS
*****************************************************************************/

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Adc driver initialization
 *  @return return true if success 
 */
bool AdcDriverInit(void);

/** @brief Get raw data from ADC
 *  @param adcChannel channel number
 *  @return return raw data
 */
uint32_t AdcDriverGetRawData(AdcDriverChannel_t adcChannel);

/** @brief Get milli voltage data from ADC
 *  @param adcChannel channel number
 *  @return return milli voltage data
 */
float AdcDriverGetMilliVoltageData(AdcDriverChannel_t adcChannel);