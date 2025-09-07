/*****************************************************************************
 * @file middleware/circBuff/circBuff.c
 *
 * @brief generic circular buffer
 *
 * @author tomplu
 * @date 20.05.2019
 * @version v1.0
 *
 * @copyright 2019 Fideltronik R&D - all rights reserved.
 ****************************************************************************/

#include "utils/circBuff/circBuff.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*****************************************************************************
                               INTERFACE IMPLEMENTATION
*****************************************************************************/

bool CircBuffDynamicBufferInit(circBuff_t *cbuf, uint32_t size)
{
    assert(cbuf);
    assert(size);
    memset(cbuf, 0, sizeof(circBuff_t));

    cbuf->data = malloc(size);

    if (cbuf->data) {
        cbuf->size = size;
        cbuf->isAllocated = true;
        return true;
    }

    return false;
}

bool CircBuffStaticBufferInit(circBuff_t *cbuf, uint8_t *dataBuf, uint32_t size)
{
    assert(cbuf);
    assert(size);
    memset(cbuf, 0, sizeof(circBuff_t));

    cbuf->data = dataBuf;
    cbuf->size = size;

    return (cbuf->data != NULL);
}

void CircBuffDeinit(circBuff_t *cbuf)
{
    assert(cbuf);

    if (cbuf->isAllocated) {
        free(cbuf->data);
    }

    memset(cbuf, 0, sizeof(circBuff_t));
}

void CircBuffWrite(circBuff_t *cbuf, uint8_t *data, uint32_t len)
{
    assert(cbuf);
    assert(data);

    if (len == 0 || len > cbuf->size) {
        return;
    }

    uint32_t avaiableStartSpace = CircBuffWriteSize(cbuf);

    uint32_t dataNoToBuffEnd = cbuf->size - cbuf->write;

    if (len <= dataNoToBuffEnd) {
        memcpy(&cbuf->data[cbuf->write], data, len);
        cbuf->write += len;
        if (cbuf->write == cbuf->size) {
            cbuf->write = 0;
        }
    }
    else {
        memcpy(&cbuf->data[cbuf->write], data, dataNoToBuffEnd);
        uint32_t left = len - dataNoToBuffEnd;
        memcpy(cbuf->data, &data[dataNoToBuffEnd], left);
        cbuf->write = left;
    }

    // Some data was overwritten!
    if (len >= avaiableStartSpace) {
        cbuf->read = cbuf->write;
    }

    if (cbuf->read == cbuf->write) {
        cbuf->full = true;
    }
}

uint32_t CircBuffPeek(circBuff_t *cbuf, uint8_t *dataOut, uint32_t lenToRead)
{
    assert(cbuf);
    assert(dataOut);

    if (lenToRead == 0 || lenToRead > cbuf->size) {
        return 0;
    }

    uint32_t unreadInBuff = CircBuffReadSize(cbuf);
    uint32_t bytesToRead = (unreadInBuff < lenToRead) ? unreadInBuff : lenToRead;

    if (bytesToRead == 0) {
        return 0;
    }

    if (cbuf->write > cbuf->read) {
        memcpy(dataOut, &cbuf->data[cbuf->read], bytesToRead);
    }
    else {
        uint32_t dataNoToBuffEnd = cbuf->size - cbuf->read;
        uint32_t firstCopy = (dataNoToBuffEnd > bytesToRead) ? bytesToRead : dataNoToBuffEnd;

        if (firstCopy > 0) {
            memcpy(dataOut, &cbuf->data[cbuf->read], firstCopy);
        }

        uint32_t secondCopy = bytesToRead - firstCopy;

        if (secondCopy > 0) {
            memcpy(dataOut + firstCopy, cbuf->data, secondCopy);
        }
    }

    return bytesToRead;
}

uint32_t CircBuffDrop(circBuff_t *cbuf, uint32_t lenToDrop)
{
    assert(cbuf);

    if (lenToDrop == 0 || lenToDrop > cbuf->size) {
        return 0;
    }

    uint32_t toReadCount = CircBuffReadSize(cbuf);
    if (toReadCount == 0) {
        return 0;
    }

    if (toReadCount > 0) {
        lenToDrop = lenToDrop > toReadCount ? toReadCount : lenToDrop;
        cbuf->read = ((cbuf->read + lenToDrop) > cbuf->size) ? ((cbuf->read + lenToDrop) - cbuf->size) :
                                                               (cbuf->read + lenToDrop);

        cbuf->full = false;
    }

    return lenToDrop;
}

uint32_t CircBuffRead(circBuff_t *cbuf, uint8_t *dataOut, uint32_t lenToRead)
{
    assert(cbuf);
    assert(dataOut);

    if (lenToRead == 0 || lenToRead > cbuf->size) {
        return 0;
    }

    uint32_t justRead = CircBuffPeek(cbuf, dataOut, lenToRead);
    CircBuffDrop(cbuf, justRead);

    return justRead;
}

uint32_t CircBuffReadSize(circBuff_t *cbuf)
{
    assert(cbuf);

    if (cbuf->write == cbuf->read) {
        if (cbuf->full) {
            return cbuf->size;
        }
        return 0;
    }

    if (cbuf->write > cbuf->read) {
        return cbuf->write - cbuf->read;
    }

    return cbuf->size - (cbuf->read - cbuf->write);
}

uint32_t CircBuffWriteSize(circBuff_t *cbuf)
{
    assert(cbuf);

    if (cbuf->write == cbuf->read) {
        if (cbuf->full) {
            return 0;
        }
        return cbuf->size;
    }

    if (cbuf->write > cbuf->read) {
        return cbuf->size - (cbuf->write - cbuf->read);
    }

    return cbuf->read - cbuf->write;
}

uint32_t CircBuffGetTotalSize(circBuff_t *cbuf)
{
    assert(cbuf);
    return cbuf->size;
}

bool CircBuffIsFull(circBuff_t *cbuf)
{
    assert(cbuf);
    if (cbuf->full) {
        return true;
    }
    return false;
}
