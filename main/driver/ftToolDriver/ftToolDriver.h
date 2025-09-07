/**
 * @file ftToolDriver.h
 *
 * @brief Implementation of FT Protocol to communicate with computer application Fideltronik Service Tool
 *
 * @author matfio
 * @date 2020.01.27
 * @version v1.0
 *
 * @copyright 2020 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/*****************************************************************************
                       PUBLIC DEFINES / MACROS
*****************************************************************************/

#define FT_TOOL_DIAG_PARAM_NAME_MAX_LEN (4U)        //!< Max parameter name length
#define FT_TOOL_DIAG_PARAM_UNIT_NAME_MAX_LEN (4U)   //!< Max unit parameter name length
#define FT_TOOL_DIAG_PARAM_DESC_MAX_LEN (32U)       //!< Max parameter description length
#define FT_TOOL_DIAG_PARAM_CHANNEL_MAX_NUMBER (8U)  //!< Max channels number in single parameter

#define FT_TOOL_FRAME_MAX_DATA_LEN (240U)  //!< Max data size in single frame

/*****************************************************************************
                       PUBLIC ENUMS
*****************************************************************************/

/** @brief Determine access type
 */
typedef enum {
    FT_TOOL_DIAG_PARAM_PERMISSION_READ_ONLY = 0,  //!< Read only parameter
    FT_TOOL_DIAG_PARAM_PERMISSION_READ_WRITE,     //!< Read and write parameter
} ftToolDiagParamPermission_t;

/** @brief Parameter type
 */
typedef enum {
    FT_TOOL_DIAG_PARAM_TYPE_STRING = 's',    //!< String
    FT_TOOL_DIAG_PARAM_TYPE_BYTE = 'b',      //!< Byte
    FT_TOOL_DIAG_PARAM_TYPE_BUTTON = 'B',    //!< Button
    FT_TOOL_DIAG_PARAM_TYPE_UNSIGNED = 'u',  //!< Unsigned integer
    FT_TOOL_DIAG_PARAM_TYPE_SIGNED = 'i'     //!< Signed integer
} ftToolDiagParamType_t;

/** @brief Completion of data frame status
 */
typedef enum {
    FT_TOOL_FRAME_COMPLEMENT_STATUS_IDLE = 0,    //!< Idle
    FT_TOOL_FRAME_COMPLEMENT_STATUS_ERROR,       //!< Error
    FT_TOOL_FRAME_COMPLEMENT_STATUS_INCOMPLETE,  //!< Incomplete
    FT_TOOL_FRAME_COMPLEMENT_STATUS_CORRECT      //!< Correct
} ftToolFrameComplementStatus_t;

/** @brief Status of frame processing
 */
typedef enum {
    FT_TOOL_PROCESS_IDLE = 0,            //!< Idle
    FT_TOOL_PROCESS_OK,                  //!< Success
    FT_TOOL_PROCESS_UNSUPORT_CMD,        //!< Unsupported  command error
    FT_TOOL_PROCESS_UMKNOWN_CMD,         //!< Unknown command error
    FT_TOOL_PROCESS_PARAM_OUT_OF_RANGE,  //!< Parameter value is out or range error
    FT_TOOL_PROCESS_READ_RETURN_ERROR,   //!< Read function return error
    FT_TOOL_PROCESS_WRITE_RETURN_ERROR,  //!< Write function return error
    FT_TOOL_PROCESS_READ_ONLY_PARAM      //!< Parameter is read only error
} ftToolProcessStatus_t;

/*****************************************************************************
                       PUBLIC STRUCTS / VARIABLES
*****************************************************************************/

/** @brief Pointer to function that read parameter value
 *  @param channel number of channel that will be read
 *  @param dataPtr pointer to read data
 *  @param dataSize read data size
 *  @return true if read success
 */
typedef bool (*ftToolReadHandler_t)(uint8_t channel, void *dataPtr, uint32_t dataSize);

/** @brief Pointer to function that write parameter value
 *  @param channel number of channel that will be write
 *  @param dataPtr pointer to write data
 *  @param dataSize write data size
 *  @return true if write success
 */
typedef bool (*ftToolWriteHandler_t)(uint8_t channel, const void *dataPtr, uint32_t dataSize);

/** @brief Single parameter definition
 */
typedef struct {
    char name[FT_TOOL_DIAG_PARAM_NAME_MAX_LEN];  //!< Name of parameter
    uint8_t permission;         //!< Determining the type of access(ftToolDiagParamPermission_t)
    uint8_t channels;           //!< Number of channels
    uint8_t fieldSize;          //!< Number of data in bytes
    char fieldFormat;           //!< Data type (ftToolDiagParamType_t)
    uint8_t multiplierBase;     //!< Multiplier base
    int8_t multiplierExponent;  //!< Multiplier exponent
    char unitName[FT_TOOL_DIAG_PARAM_UNIT_NAME_MAX_LEN];  //!< Unit name
    char description[FT_TOOL_DIAG_PARAM_DESC_MAX_LEN];    //!< Description of parameter
} ftToolDiagParam_t;

/** @brief Extend parameter definition
 */
typedef struct {
    ftToolDiagParam_t diagParam;        //!< Parameter definition
    ftToolReadHandler_t readHandler;    //!< Read parameter function handler
    ftToolWriteHandler_t writeHandler;  //!< Write parameter function handler
} FtToolParamExec_t;

/** @brief Single frame definition
 */
typedef struct {
    uint8_t address;                           //!< Address (hard coded 0x00)
    uint8_t id;                                //!< Index of read write parameter
    uint8_t command;                           //!< Command type
    uint8_t status;                            //!< Operation status
    uint8_t dataSize;                          //!< Data size
    uint8_t data[FT_TOOL_FRAME_MAX_DATA_LEN];  //!< Data to send or read
    uint8_t crc;                               //!< Cyclic redundancy check
} ftToolFrame_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Check if parameters array are set correctly and initialize
 *  @param diagParamExec pointer to parameters array
 *  @param paramCount number of elements in array
 *  @return true if it succeeded or false
 */
bool FtToolInit(const FtToolParamExec_t *diagParamExec, uint8_t paramCount);

/** @brief Complement FT frame from raw data
 *  @param singleRawData raw data
 *  @param ftToolFrame pointer to frame with data
 *  @return status of created frame
 */
ftToolFrameComplementStatus_t FtToolFrameComplement(uint8_t singleRawData, ftToolFrame_t *ftToolFrame);

/** @brief Processing received frame
 *  @param ftToolFrame pointer to received frame and response to send
 *  @return status of created frame
 */
ftToolProcessStatus_t FtToolProcess(ftToolFrame_t *ftToolFrame);

/** @brief Prepare Data to send
 *  @param ftToolFrame pointer to received data frame
 *  @param outBuff pointer to buffer with prepared data to send
 *  @return number data to send
 */
uint16_t FtToolPrepareOutputBuffer(ftToolFrame_t *ftToolFrame, uint8_t *outBuff);
