/*****************************************************************************
 * @file multiBuffer.h
 *
 * @brief Module that handle multiple-buffering
 *
 * @author matfio
 * @date 2019.07.22
 * @version v1.0
 *
 * @copyright 2019 Fideltronik R&D - all rights reserved.
 ****************************************************************************/
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/
#ifndef CFG_MULTI_BUFF_MAX_COUNT
#define CFG_MULTI_BUFF_MAX_COUNT (128U)
#endif

typedef enum {
    SINGLE_BUFFER_EMPTY = 0,
    SINGLE_BUFFER_CONTAIN_DATA,
    SINGLE_BUFFER_FULL
} singleBufferStatus_t;

/*****************************************************************************
                     PUBLIC STRUCTS / VARIABLES
*****************************************************************************/

typedef struct {
    uint8_t *buffer;
    uint32_t size;
    singleBufferStatus_t status;
} __attribute__ ((packed)) singleBuffer_t;

typedef struct {
    singleBuffer_t singleBuffer[CFG_MULTI_BUFF_MAX_COUNT];
    uint8_t actualBufferIdx;
    uint32_t maxBufferSize;
    uint8_t bufferCount;
    bool initByMalloc;
} __attribute__ ((packed)) multipleBuffer_t;

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

/** @brief Initialize multiple Buffer using given parameters and dynamically allocate memory
 *  @param handle pointer to multipleBuffer_t structure
 *  @param mBuffSize max size of any buffer
 *  @param mBCount number of available buffers
 *  @return true if it succeeded or false
 */
bool MultiBufferCreateDynamic(multipleBuffer_t *handler, uint32_t mBuffSize, uint8_t mBCount);

/** @brief Initialize multiple Buffer using given parameter
 *  @param handle pointer to multipleBuffer_t structure
 *  @param buffer pointer to static memory space
 *  @param mBuffSize max size of any buffer
 *  @param mBCount number of available buffers
 *  @return true if it succeeded or false
 */
bool MultiBufferCreateStatic(multipleBuffer_t *handler, uint8_t *buffer, uint32_t mBuffSize, uint8_t mBCount);

/** @brief Free dynamically allocate memory
 *  @param handle pointer to multipleBuffer_t structure
 */
void MultiBufferDeinit(multipleBuffer_t *handler);

/** @brief Get max size of any buffer
 *  @param handle pointer to multipleBuffer_t structure
 *  @return max size
 */
uint32_t MultiBufferGetMaxSize(const multipleBuffer_t *handler);

/** @brief Get number of available buffers
 *  @param handle pointer to multipleBuffer_t structure
 *  @return number of buffers
 */
uint8_t MultiBufferGetCount(const multipleBuffer_t *handler);

/** @brief Get actual size of current buffer
 *  @param handle pointer to multipleBuffer_t structure
 *  @return actual size
 */
uint32_t MultiBufferGetCurrentBufferSize(const multipleBuffer_t *handler);

/** @brief Get number of buffers marked as "full"
 *  @param handle pointer to multipleBuffer_t structure
 *  @return number buffers marked as "full"
 */
uint8_t MultiBufferGetFullBufferCount(const multipleBuffer_t *handler);

/** @brief Read data from first buffer marked as "full"
 *  @param handle pointer to multipleBuffer_t structure
 *  @param data pointer to read data
 *  @param bufferSize pointer that contain read buffer size
 *  @param bufferNumber pointer that contain actual read buffer number
 *  @return true or false if there is no "full" buffer
 */
bool MultiBufferGetFirstFullBuffer(multipleBuffer_t *handler, uint8_t *data, uint32_t *bufferSize,
                                   uint8_t *bufferNumber);

/** @brief Marks given buffer number as "empty"
 *  @param handle pointer to multipleBuffer_t structure
 *  @param bufferNumber number of buffer
 *  @return true or false when buffer is already marked "empty"
 */
bool MultiBufferMarkEmpty(multipleBuffer_t *handler, uint8_t bufferNumber);

/** @brief Switches to next buffer that is not "full" and not "empty"
 *  @param handle pointer to multipleBuffer_t structure
 *  @return true or false all buffers are full or current write buffer is "empty"
 */
bool MultiBufferSwitchNext(multipleBuffer_t *handler);

/** @brief Write data to buffer
 *  @param handle pointer to multipleBuffer_t structure
 *  @param data pointer to data
 *  @param dataSize
 *  @return wrote data number or 0 when write is impossible (all buffers are "full")
 */
uint32_t MultiBufferWrite(multipleBuffer_t *handler, uint8_t *data, uint32_t dataSize);

/** @brief Mark "empty" first buffer marked as "full"
 *  @param handle pointer to multipleBuffer_t structure
 *  @param bufferNumber pointer that contain mark "empty" buffer number
 *  @return true or false if there is no "full" buffer
 */
bool MultiBufferMarkEmptyFirstFullBuffer(multipleBuffer_t *handler, uint8_t *bufferNumber);
