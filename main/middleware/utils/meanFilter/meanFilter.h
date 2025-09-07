/*****************************************************************************
 * @file meanFilter/meanFilter.h
 *
 * @brief Header file template
 *
 * @author micfra
 * @date 09.01.2020
 * @version v1.0
 *
 * @copyright 2019 Fideltronik R&D - all rights reserved.
 ****************************************************************************/
#pragma once

#include <stdbool.h>
#include <stdint.h>

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

typedef struct {
    uint32_t bufferSize;
    uint32_t *buffer;
    uint32_t currentIndex;
    uint32_t bufferSumm;
    bool beginningState;
} meanFilter_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/**@brief Initializes mean filter and allocates memory for it's buffer
 *
 * @param filterStruct pointer to meanFilter_t structure
 * @param bufferSize size of the filter buffer
 * @return true if success
 */
bool MeanFilterInit(meanFilter_t *filterStruct, uint32_t bufferSize);

/**@brief Performs data filtering
 *
 * @param filterStruct pointer to meanFilter_t structure
 * @param data for filtering
 * @return filtered data
 */
uint32_t MeanFilterFilterData(meanFilter_t *filterStruct, uint32_t data);

/**@brief Frees filter buffer pointer
 *
 * @param filterStruct pointer to meanFilter_t structure
 */
void MeanFilterDeinit(meanFilter_t *filterStruct);
