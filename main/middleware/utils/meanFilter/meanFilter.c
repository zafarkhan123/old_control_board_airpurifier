/*****************************************************************************
 * @file meanFilter/meanFilter.c
 *
 * @brief Source code file template
 *
 * @author micfra
 * @date 09.01.2020
 * @version v1.0
 *
 * @copyright 2019 Fideltronik R&D - all rights reserved.
 ****************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "utils/meanFilter/meanFilter.h"

#include <assert.h>

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool MeanFilterInit(meanFilter_t *filterStruct, uint32_t bufferSize)
{
    assert(filterStruct);

    if (bufferSize == 0) {
        return false;
    }

    filterStruct->bufferSize = bufferSize;
    filterStruct->buffer = calloc(bufferSize, sizeof(uint32_t));
    if (filterStruct->buffer == NULL) {
        return false;
    }

    filterStruct->currentIndex = 0;
    filterStruct->bufferSumm = 0;
    filterStruct->beginningState = true;

    return true;
}

uint32_t MeanFilterFilterData(meanFilter_t *filterStruct, uint32_t data)
{

    filterStruct->bufferSumm -= filterStruct->buffer[filterStruct->currentIndex];
    filterStruct->bufferSumm += data;
    filterStruct->buffer[filterStruct->currentIndex] = data;


    filterStruct->currentIndex += 1;

    if (filterStruct->beginningState) {
        uint32_t output = filterStruct->bufferSumm / filterStruct->currentIndex;

        if (filterStruct->currentIndex >= filterStruct->bufferSize) {
            filterStruct->currentIndex = 0;
            filterStruct->beginningState = false;
        }
        return output;
    }

    if (filterStruct->currentIndex >= filterStruct->bufferSize) {
        filterStruct->currentIndex = 0;
    }

    return filterStruct->bufferSumm / filterStruct->bufferSize;
}

void MeanFilterDeinit(meanFilter_t *filterStruct)
{
    free(filterStruct->buffer);
    memset(filterStruct, 0, sizeof(meanFilter_t));
}
