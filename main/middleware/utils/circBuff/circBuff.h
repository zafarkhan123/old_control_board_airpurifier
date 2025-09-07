/*****************************************************************************
 * @file middleware/circBuff/circBuff.h
 *
 * @brief generic circular buffer
 *
 * @author tomplu
 * @date 20.05.2019
 * @version v1.0
 *
 * @copyright 2019 Fideltronik R&D - all rights reserved.
 ****************************************************************************/
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

typedef struct {
    uint8_t *data;
    uint32_t read;
    uint32_t write;
    uint32_t size;
    bool full;
    bool isAllocated;
} circBuff_t;


/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initlizes circular buffer and allocates memory with given size
 *  @param cbuf - circular buffer
 *  @param size - data buffer size
 *  @return true - if memory allocation suceeded, false if not
 */
bool CircBuffDynamicBufferInit(circBuff_t *cbuf, uint32_t size);

/** @brief Initlizes circular buffer with pre-allocated data buffer
 *  @param cbuf - circular buffer
 *  @param dataBuf - pre-allocated data buffer
 *  @param size - cbuf size for data
 *  @return true - if correctly initialized
 */
bool CircBuffStaticBufferInit(circBuff_t *cbuf, uint8_t *dataBuf, uint32_t size);

/** @brief Deinitializes the buffer. If memory was allocated, it is freed.
 *  @param cbuf - circular buffer
 */
void CircBuffDeinit(circBuff_t *cbuf);


/** @brief Write "len" bytes to buffer (but no more than buffer size)
 *         If buffer space is smaller than "len", oldest data is overwritten
 *  @param cbuf - circular buffer
 *  @param data - data to write
 *  @param len - data count to write, but no more that buffer size
 */
void CircBuffWrite(circBuff_t *cbuf, uint8_t *data, uint32_t len);


/** @brief Read "lenToRead" data from buffer.
 *
 *  Maximum length to read cannot exceed buffer size.
 *  If less data is available, less data will be read.
 *
 *  @param cbuf - circular buffer
 *  @param dataOut - data output buffer, should be as big as least "lenToRead"
 *  @param lenToRead - amount of data to read
 *  @return Data actually read
 */
uint32_t CircBuffRead(circBuff_t *cbuf, uint8_t *dataOut, uint32_t lenToRead);


/** @brief Copy "lenToRead" data into "dataOut" buffer without removing them
 *         from the buffer.
 *  @param cbuf - circular buffer
 *  @param dataOut - data output buffer, should be as big as least "lenToRead"
 *  @param lenToRead - amount of data to copy
 *  @return Data actually copied
 */
uint32_t CircBuffPeek(circBuff_t *cbuf, uint8_t *dataOut, uint32_t lenToRead);


/** @brief Drops data from buffer (marks given amount of data as read)
 *  @param cbuf - circular buffer
 *  @param lenToDrop - data count to drop
 *  @return Data actually dropped
 */
uint32_t CircBuffDrop(circBuff_t *cbuf, uint32_t lenToDrop);


/** @brief Returns unread data count.
 *  @param cbuf - circular buffer
 *  @return unread data count
 */
uint32_t CircBuffReadSize(circBuff_t *cbuf);


/** @brief Returns cbuf size - max data count to write with no overflow
 *  @param cbuf - circular buffer
 *  @return possible to write data number
 */
uint32_t CircBuffWriteSize(circBuff_t *cbuf);


/** @brief Returns cbuf size - max data count to hold in
 *  @param cbuf - circular buffer
 *  @return cbuf size
 */
uint32_t CircBuffGetTotalSize(circBuff_t *cbuf);

/** @brief Returns true if the buffer is full
 *  @param cbuf - circular buffer
 *  @return  true if the buffer is full, otherwise false
 */
bool CircBuffIsFull(circBuff_t *cbuf);
