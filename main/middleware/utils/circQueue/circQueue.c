/*****************************************************************************
 * @file circQueue.c
 *
 * @brief  generic circular queue for circular buffering structure types
 *
 * @author  matfio
 * @date 23.09.2019
 * @version v1.0
 *
 * @copyright 2019 Fideltronik R&D - all rights reserved.
 ****************************************************************************/

#include <assert.h>

#include "utils/circQueue/circQueue.h"

bool CircQueueStaticBufferInit(circQueue_t *handler, void *dataBuf, uint32_t bufferSize, uint16_t recordSize)
{
    assert(recordSize != 0);
    assert((bufferSize % recordSize) == 0);		// buffer size should have non fractional capacity for records
    assert(bufferSize / recordSize > 1);

    bool status = CircBuffStaticBufferInit(&handler->circBuff, dataBuf, bufferSize);

    assert(status);

    handler->dropElementNumber = 0;
    handler->recordSize = recordSize;

    return true;
}

void CircQueueWrite(circQueue_t *handler, void *element)
{
    if (CircBuffWriteSize(&handler->circBuff) == 0) {
        handler->dropElementNumber++;
    }

    CircBuffWrite(&handler->circBuff, element, handler->recordSize);
}

uint16_t CircQueueRead(circQueue_t *handler, void *elements, uint16_t elementsToRead)
{
    uint32_t lenToRead = handler->recordSize * elementsToRead;

    uint32_t actualRead = CircBuffRead(&handler->circBuff, elements, lenToRead);

    assert((actualRead % handler->recordSize) == 0);

    return actualRead / handler->recordSize;
}

uint16_t CircQueuePeek(circQueue_t *handler, void *elements, uint16_t elementsToRead)
{
    uint32_t lenToRead = handler->recordSize * elementsToRead;

    uint32_t actualRead = CircBuffPeek(&handler->circBuff, elements, lenToRead);

    assert((actualRead % handler->recordSize) == 0);

    return actualRead / handler->recordSize;
}

uint16_t CircQueueDrop(circQueue_t *handler, uint16_t elementsToDrop)
{
    uint32_t lenToDrop = handler->recordSize * elementsToDrop;

    uint32_t actualDrop = CircBuffDrop(&handler->circBuff, lenToDrop);

    assert((actualDrop % handler->recordSize) == 0);

    return actualDrop / handler->recordSize;
}

uint16_t CircQueueReadSize(circQueue_t *handler)
{
    uint32_t readSize = CircBuffReadSize(&handler->circBuff);

    return readSize / handler->recordSize;
}

uint16_t CircQueueWriteSize(circQueue_t *handler)
{
    uint32_t writeSize = CircBuffWriteSize(&handler->circBuff);

    return writeSize / handler->recordSize;
}

uint16_t CircQueueDropElementNumber(circQueue_t *handler)
{
    return handler->dropElementNumber;
}
