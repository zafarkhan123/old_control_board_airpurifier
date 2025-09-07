/*****************************************************************************
 * @file circQueue.h
 *
 * @brief  generic circular queue for circular buffering structure types
 *
 * @author  matfio
 * @date 23.09.2019
 * @version v1.0
 *
 * @copyright 2019 Fideltronik R&D - all rights reserved.
 ****************************************************************************/
#pragma once

#include <stdbool.h>

#include "utils/circBuff/circBuff.h"

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/
typedef struct {
    circBuff_t circBuff;
    uint16_t recordSize;
    uint16_t dropElementNumber;
} circQueue_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initializes circular queue with preallocated buffer (static)
 *  @param handler - circular Queue handler
 *  @param buffer - pre-allocated data buffer ( f.ex.some struct data type)
 *  @param bufferSize - size of data buffer ,shout be multiplication of elementSize and max amount of elements you wont to add
 *  @param elementSize - sizeof one element which is stored in circQueue
 *  @return true - if correctly initialized
 */
bool CircQueueStaticBufferInit(circQueue_t *handler, void *dataBuf, uint32_t bufferSize, uint16_t elementSize);

/** @brief Write one element to circular queue
 *  @param handler - circular Queue handler
 *  @param element - element to write
 */
void CircQueueWrite(circQueue_t *handler, void *element);

/** @brief Read "elementsToRead" records from buffer
 *  @param handler - circular Queue handler
 *  @param dataOut - pointer to read buffer
 *  @return number of read elements
 */
uint16_t CircQueueRead(circQueue_t *handler, void *element, uint16_t elementsToRead);

/** @brief Copy "elementsToRead" data into "dataOut" buffer without removing them
 *         from the buffer.
 *  @param handler - circular queue
 *  @param elements - pointer to read buffer
 *  @param elementsToRead - amount of elements to copy
 *  @return number of  elements actually copied
 */
uint16_t CircQueuePeek(circQueue_t *handler, void *elements, uint16_t elementsToRead);

/** @brief Drops data from buffer (marks given amount of data as read)
 *  @param handler - circular queue
 *  @param elementsToDrop - number of elements to drop
 *  @return Elements actually dropped
 */
uint16_t CircQueueDrop(circQueue_t *handler, uint16_t elementsToDrop);

/** @brief Returns unread elements from buffer.
 *  @param handler - circular queue
 *  @return unread elements count
 */
uint16_t CircQueueReadSize(circQueue_t *handler);

/** @brief Returns max elements to write with no overflow
 *  @param handler - circular queue
 *  @return possible to write elements number
 */
uint16_t CircQueueWriteSize(circQueue_t *handler);

/** @brief Returns dropped elements number
 *  @param handler - circular queue
 *  @return dropped elements number
 */
uint16_t CircQueueDropElementNumber(circQueue_t *handler);
