/*****************************************************************************
 * @file multiBuffer.c
 *
 * @brief Module that handle multiple-buffering
 *
 * @author matfio
 * @date 2019.07.22
 * @version v1.0
 *
 * @copyright 2019 Fideltronik R&D - all rights reserved.
 ****************************************************************************/
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "multiBuffer.h"
#include <assert.h>

#include "config.h"

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool MultiBufferCreateDynamic(multipleBuffer_t *handler, uint32_t mBuffSize, uint8_t mBCount)
{
    assert(handler);
    assert(mBuffSize);
    assert(mBCount > 1);
    assert(mBCount <= CFG_MULTI_BUFF_MAX_COUNT);

    memset(handler, 0, sizeof(multipleBuffer_t));

    handler->bufferCount = mBCount;
    handler->maxBufferSize = mBuffSize;
    handler->initByMalloc = true;

    for (uint8_t buffIdx = 0; buffIdx < handler->bufferCount; ++buffIdx) {
        handler->singleBuffer[buffIdx].buffer = (uint8_t *)malloc(handler->maxBufferSize);
        handler->singleBuffer[buffIdx].status = SINGLE_BUFFER_EMPTY;
        if (handler->singleBuffer[buffIdx].buffer == NULL) {
            MultiBufferDeinit(handler);
            return false;
        }
    }

    return true;
}

bool MultiBufferCreateStatic(multipleBuffer_t *handler, uint8_t *buffer, uint32_t mBuffSize, uint8_t mBCount)
{
    assert(handler);
    assert(buffer);
    assert(mBuffSize);
    assert(mBCount > 1);
    assert(mBCount <= CFG_MULTI_BUFF_MAX_COUNT);

    memset(handler, 0, sizeof(multipleBuffer_t));

    for (uint8_t buffIdx = 0; buffIdx < mBCount; ++buffIdx) {
        handler->singleBuffer[buffIdx].buffer = &buffer[buffIdx * mBuffSize];
        handler->singleBuffer[buffIdx].status = SINGLE_BUFFER_EMPTY;
    }

    handler->bufferCount = mBCount;
    handler->maxBufferSize = mBuffSize;
    handler->initByMalloc = false;

    return true;
}

void MultiBufferDeinit(multipleBuffer_t *handler)
{
    assert(handler);

    if (!handler->initByMalloc) {
        return;
    }

    for (uint8_t buffIdx = 0; buffIdx < handler->bufferCount; ++buffIdx) {
        if (handler->singleBuffer[buffIdx].buffer == NULL) {
            continue;
        }

        free(handler->singleBuffer[buffIdx].buffer);
        handler->singleBuffer[buffIdx].buffer = NULL;
    }
}

uint32_t MultiBufferGetMaxSize(const multipleBuffer_t *handler)
{
    assert(handler);

    return handler->maxBufferSize;
}

uint8_t MultiBufferGetCount(const multipleBuffer_t *handler)
{
    assert(handler);

    return handler->bufferCount;
}

uint32_t MultiBufferGetCurrentBufferSize(const multipleBuffer_t *handler)
{
    assert(handler);

    return handler->singleBuffer[handler->actualBufferIdx].size;
}

uint8_t MultiBufferGetFullBufferCount(const multipleBuffer_t *handler)
{
    assert(handler);

    uint8_t countFullBuff = 0;

    for (uint8_t buffIdx = 0; buffIdx < handler->bufferCount; ++buffIdx) {
        if (handler->singleBuffer[buffIdx].status == SINGLE_BUFFER_FULL) {
            ++countFullBuff;
        }
    }

    return countFullBuff;
}

bool MultiBufferGetFirstFullBuffer(multipleBuffer_t *handler, uint8_t *data, uint32_t *bufferSize, uint8_t *bufferNumber)
{
    assert(handler);
    assert(data);
    assert(bufferSize);
    assert(bufferNumber);

    uint8_t nextIdx = handler->actualBufferIdx + 1;
    nextIdx %= handler->bufferCount;
    uint8_t checkedCount = 0;

    while (checkedCount < (handler->bufferCount - 1)) {
        if (handler->singleBuffer[nextIdx].status == SINGLE_BUFFER_FULL) {
            *bufferSize = handler->singleBuffer[nextIdx].size;
            memcpy(data, handler->singleBuffer[nextIdx].buffer, *bufferSize);
            *bufferNumber = nextIdx;
            return true;
        }
        ++checkedCount;
        ++nextIdx;
        nextIdx %= handler->bufferCount;
    }

    return false;
}

bool MultiBufferMarkEmpty(multipleBuffer_t *handler, uint8_t bufferNumber)
{
    assert(handler);
    assert(bufferNumber < handler->bufferCount);

    if (handler->singleBuffer[bufferNumber].status == SINGLE_BUFFER_EMPTY) {
        return false;
    }

    handler->singleBuffer[bufferNumber].status = SINGLE_BUFFER_EMPTY;
    handler->singleBuffer[bufferNumber].size = 0;

    return true;
}

bool MultiBufferSwitchNext(multipleBuffer_t *handler)
{
    assert(handler);

    if (handler->singleBuffer[handler->actualBufferIdx].status == SINGLE_BUFFER_EMPTY) {
        return false;
    }

    handler->singleBuffer[handler->actualBufferIdx].status = SINGLE_BUFFER_FULL;

    for (uint8_t buffIdx = 0; buffIdx < handler->bufferCount; ++buffIdx) {
        if (handler->singleBuffer[buffIdx].status != SINGLE_BUFFER_FULL) {

            handler->actualBufferIdx = buffIdx;

            return true;
        }
    }

    return false;
}

uint32_t MultiBufferWrite(multipleBuffer_t *handler, uint8_t *data, uint32_t dataSize)
{
    assert(handler);
    assert(data);
    assert(dataSize > 0);
    assert(dataSize <= handler->maxBufferSize);

    uint32_t currentSize = MultiBufferGetCurrentBufferSize(handler);

    if (currentSize + dataSize > handler->maxBufferSize) {

        handler->singleBuffer[handler->actualBufferIdx].status = SINGLE_BUFFER_FULL;

        if (MultiBufferSwitchNext(handler) == false) {
            return 0;
        }

        currentSize = MultiBufferGetCurrentBufferSize(handler);
    }

    if (dataSize == handler->maxBufferSize) {
        uint8_t *writePos = &handler->singleBuffer[handler->actualBufferIdx].buffer[currentSize];

        memcpy(writePos, data, dataSize);

        handler->singleBuffer[handler->actualBufferIdx].size += dataSize;
        handler->singleBuffer[handler->actualBufferIdx].status = SINGLE_BUFFER_FULL;

        MultiBufferSwitchNext(handler);
    }
    else {
        uint8_t *writePos = &handler->singleBuffer[handler->actualBufferIdx].buffer[currentSize];

        memcpy(writePos, data, dataSize);

        handler->singleBuffer[handler->actualBufferIdx].size += dataSize;
        handler->singleBuffer[handler->actualBufferIdx].status = SINGLE_BUFFER_CONTAIN_DATA;
    }

    return dataSize;
}

bool MultiBufferMarkEmptyFirstFullBuffer(multipleBuffer_t *handler, uint8_t *bufferNumber)
{
    assert(handler);
    assert(bufferNumber);

    uint8_t nextIdx = handler->actualBufferIdx + 1;
    nextIdx %= handler->bufferCount;

    uint8_t checkedCount = 0;

    while (checkedCount < (handler->bufferCount - 1)) {
        if (handler->singleBuffer[nextIdx].status == SINGLE_BUFFER_FULL) {
            *bufferNumber = nextIdx;
            handler->singleBuffer[nextIdx].status = SINGLE_BUFFER_EMPTY;
            handler->singleBuffer[nextIdx].size = 0;

            return true;
        }
        ++checkedCount;
        ++nextIdx;
        nextIdx %= handler->bufferCount;
    }

    return false;
}
