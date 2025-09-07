/**
 * @file ftToolDriver.c
 *
 * @brief Implementation of FT Protocol to communicate with computer application Fideltronik Service Tool
 *
 * @author matfio
 * @date 2020.01.27
 * @version v1.0
 *
 * @copyright 2020 Fideltronik R&D - all rights reserved.
 */

#include "ftToolDriver.h"
#include <string.h>

/*****************************************************************************
                     PRIVATE DEFINES / MACROS
*****************************************************************************/

#define FRAME_START_SIGN                                                                           \
    (0x02)  //!< STX unique start frame character. It can be only at beginning of the data frame.
#define FRAME_END_SIGN                                                                             \
    (0x03)  //!< ETX unique end frame character. It can be only at end of the data frame.
#define FRAME_COMPENSATION_VALUE                                                                   \
    (0x10)  //!< DLE unique compensation character. Can not be found in frame.
#define FRAME_SPECIAL_SIGN                                                                         \
    (0x10)  //!< If STX or ETX or DLE appear in frame we add FRAME_SPECIAL_SIGN value to them and add FRAME_SPECIAL_SIGN before.

#define DEFAULT_FRAME_ADDRESS_VALUE                                                                \
    (0x00)  //!< Hard coded , ftTool not support other value in address places.

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

/** @brief FT Protocol command specification
 */
typedef enum {
    FRAME_COMMAND_NAME_SET_VALUE_BY_NUMBER = 'S',  //!< Set value of parameter with the given index number
    FRAME_COMMAND_NAME_SET_VALUE_BY_NAME = 's',  //!< Set value of parameter with the given name (unsupported)
    FRAME_COMMAND_NAME_GET_VALUE_BY_NUMBER = 'G',  //!< Get value of parameter with the given index number
    FRAME_COMMAND_NAME_GET_VALUE_BY_NAME = 'g',  //!< Get value of parameter with the given name (unsupported)
    FRAME_COMMAND_NAME_GET_PARAM_NUMBER = 'P',     //!< Get number of parameters to read
    FRAME_COMMAND_NAME_GET_PARAM_DEFINITION = 'p'  //!< Get definition of parameter
} frameCommandName_t;

/** @brief Frame parts (enume used to completion a data frame)
 */
typedef enum {
    FRAME_PART_UNSET = 0,  //!< No start frame character (STX) found
    FRAME_PART_ADDRESS,    //!< STX character found. Waiting for the address number
    FRAME_PART_ID,         //!< Address is know. Waiting for the part id number
    FRAME_PART_COMMAND,    //!< Part id is know. Waiting for the command number
    FRAME_PART_STATUS,     //!< Command is know. Waiting for the status number
    FRAME_PART_DATA_SIZE,  //!< Status is know. Waiting for the data size value
    FRAME_PART_DATA,       //!< Data size is know. Waiting for read all data
    FRAME_PART_CRC,        //!< All data read. Waiting for crc
    FRAME_PART_END         //!< The data frame is completed
} framePart_t;

/** @brief Structure used to compose the data frame
 */
typedef struct {
    framePart_t framePart;      //!< Show which frame parts is complete
    uint8_t frameDataIdx;       //!< Actual frame data index
    bool isProhibitedSignShow;  //!< Is reserved (STX, ETX, BLE) sign show in frame
} frameFolding_t;

/** @brief Pointer to defined parameter list
 */
static FtToolParamExec_t *sParamExec;

/** @brief Size of parameter list
 */
static uint8_t sParamExecCount;

/** @brief Compose the data frame
 */
static frameFolding_t sFrameFoldingStatus;

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Check if parameter type is correct
 *  @param paramType parameter type to check
 *  @return true if correct
 */
static bool IsDiagParamTypeCorrect(ftToolDiagParamType_t paramType);

/** @brief Calculate frame crc
 *  @param ftToolFrame Complete data frame structure
 *  @return calculated crc from frame
 */
static uint8_t FrameCrcCalc(ftToolFrame_t *ftToolFrame);

/** @brief Check if command number is correct
 *  @param command to check
 *  @return true if correct
 */
static bool IsFrameCommandCorrect(uint8_t command);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool FtToolInit(const FtToolParamExec_t *diagParamExec, uint8_t paramCount)
{
    if ((diagParamExec == NULL) || (paramCount == 0)) {
        return false;
    }

    for (uint8_t idx = 0; idx < paramCount; idx++) {
        if (diagParamExec[idx].diagParam.permission == FT_TOOL_DIAG_PARAM_PERMISSION_READ_ONLY) {
            if ((diagParamExec[idx].readHandler == NULL) || (diagParamExec[idx].writeHandler != NULL)) {
                return false;
            }
        }
        else if (diagParamExec[idx].diagParam.permission == FT_TOOL_DIAG_PARAM_PERMISSION_READ_WRITE) {
            if ((diagParamExec[idx].readHandler == NULL) || (diagParamExec[idx].writeHandler == NULL)) {
                return false;
            }
        }
        else {
            return false;
        }

        if (diagParamExec[idx].diagParam.channels > FT_TOOL_DIAG_PARAM_CHANNEL_MAX_NUMBER) {
            return false;
        }

        if (diagParamExec[idx].diagParam.channels * diagParamExec[idx].diagParam.fieldSize > FT_TOOL_FRAME_MAX_DATA_LEN) {
            return false;
        }

        if (IsDiagParamTypeCorrect(diagParamExec[idx].diagParam.fieldFormat) == false) {
            return false;
        }
    }

    sParamExec = (FtToolParamExec_t *)diagParamExec;
    sParamExecCount = paramCount;

    return true;
}

uint16_t FtToolPrepareOutputBuffer(ftToolFrame_t *ftToolFrame, uint8_t *outBuff)
{
    uint8_t frameHeaderSize = (uint8_t *)ftToolFrame->data - (uint8_t *)ftToolFrame;

    uint8_t dataSizeWithoutCrc = frameHeaderSize + ftToolFrame->dataSize;

    uint16_t buffSize = 0;
    outBuff[buffSize] = FRAME_START_SIGN;
    buffSize++;

    uint8_t *frameDataPtr = (uint8_t *)ftToolFrame;

    for (uint16_t idx = 0; idx < dataSizeWithoutCrc; idx++, buffSize++) {
        if ((frameDataPtr[idx] == FRAME_START_SIGN) || (frameDataPtr[idx] == FRAME_END_SIGN) ||
            ((frameDataPtr[idx] == FRAME_SPECIAL_SIGN))) {

            outBuff[buffSize] = FRAME_SPECIAL_SIGN;
            buffSize++;
            outBuff[buffSize] = frameDataPtr[idx] + FRAME_COMPENSATION_VALUE;
        }
        else {
            outBuff[buffSize] = frameDataPtr[idx];
        }
    }

    if ((ftToolFrame->crc == FRAME_START_SIGN) || (ftToolFrame->crc == FRAME_END_SIGN) ||
        ((ftToolFrame->crc == FRAME_SPECIAL_SIGN))) {

        outBuff[buffSize] = FRAME_SPECIAL_SIGN;
        buffSize++;
        outBuff[buffSize] = ftToolFrame->crc + FRAME_COMPENSATION_VALUE;
    }
    else {
        outBuff[buffSize] = ftToolFrame->crc;
    }

    buffSize++;

    outBuff[buffSize] = FRAME_END_SIGN;
    buffSize++;

    return buffSize;
}

ftToolFrameComplementStatus_t FtToolFrameComplement(uint8_t singleRawData, ftToolFrame_t *ftToolFrame)
{
    uint8_t rawData = singleRawData;

    if (rawData == FRAME_START_SIGN) {
        sFrameFoldingStatus.framePart = FRAME_PART_ADDRESS;

        return FT_TOOL_FRAME_COMPLEMENT_STATUS_INCOMPLETE;
    }

    if (rawData == FRAME_END_SIGN) {
        if ((sFrameFoldingStatus.framePart == FRAME_PART_END) &&
            (FrameCrcCalc(ftToolFrame) == ftToolFrame->crc)) {
            memset(&sFrameFoldingStatus, 0, sizeof(frameFolding_t));

            return FT_TOOL_FRAME_COMPLEMENT_STATUS_CORRECT;
        }
        memset(&sFrameFoldingStatus, 0, sizeof(frameFolding_t));

        return FT_TOOL_FRAME_COMPLEMENT_STATUS_ERROR;
    }

    if (singleRawData == FRAME_SPECIAL_SIGN) {
        sFrameFoldingStatus.isProhibitedSignShow = true;
        if (sFrameFoldingStatus.framePart == FRAME_PART_UNSET) {
            return FT_TOOL_FRAME_COMPLEMENT_STATUS_IDLE;
        }

        return FT_TOOL_FRAME_COMPLEMENT_STATUS_INCOMPLETE;
    }

    if (sFrameFoldingStatus.isProhibitedSignShow) {
        rawData -= FRAME_COMPENSATION_VALUE;
        sFrameFoldingStatus.isProhibitedSignShow = false;
    }

    ftToolFrameComplementStatus_t status = FT_TOOL_FRAME_COMPLEMENT_STATUS_INCOMPLETE;

    switch (sFrameFoldingStatus.framePart) {
    case FRAME_PART_UNSET: {
        status = FT_TOOL_FRAME_COMPLEMENT_STATUS_IDLE;
        break;
    }
    case FRAME_PART_ADDRESS: {
        ftToolFrame->address = rawData;
        sFrameFoldingStatus.framePart = FRAME_PART_ID;

        break;
    }
    case FRAME_PART_ID: {
        ftToolFrame->id = rawData;
        sFrameFoldingStatus.framePart = FRAME_PART_COMMAND;

        break;
    }
    case FRAME_PART_COMMAND: {
        if (IsFrameCommandCorrect(rawData) == false) {
            memset(&sFrameFoldingStatus, 0, sizeof(frameFolding_t));

            status = FT_TOOL_FRAME_COMPLEMENT_STATUS_ERROR;
        }
        else {
            ftToolFrame->command = rawData;
            sFrameFoldingStatus.framePart = FRAME_PART_STATUS;
        }
        break;
    }
    case FRAME_PART_STATUS: {
        ftToolFrame->status = rawData;
        sFrameFoldingStatus.framePart = FRAME_PART_DATA_SIZE;

        break;
    }

    case FRAME_PART_DATA_SIZE: {
        if (rawData >= FT_TOOL_FRAME_MAX_DATA_LEN) {
            memset(&sFrameFoldingStatus, 0, sizeof(frameFolding_t));

            status = FT_TOOL_FRAME_COMPLEMENT_STATUS_ERROR;
        }
        else {
            ftToolFrame->dataSize = rawData;
            if (ftToolFrame->dataSize == 0) {
                sFrameFoldingStatus.framePart = FRAME_PART_CRC;
            }
            else {
                sFrameFoldingStatus.framePart = FRAME_PART_DATA;
            }
        }
        break;
    }
    case FRAME_PART_DATA: {
        ftToolFrame->data[sFrameFoldingStatus.frameDataIdx] = rawData;
        sFrameFoldingStatus.frameDataIdx++;
        if (sFrameFoldingStatus.frameDataIdx >= ftToolFrame->dataSize) {
            sFrameFoldingStatus.framePart = FRAME_PART_CRC;
        }

        break;
    }
    case FRAME_PART_CRC: {
        ftToolFrame->crc = rawData;
        sFrameFoldingStatus.framePart = FRAME_PART_END;

        break;
    }
    default: {
        memset(&sFrameFoldingStatus, 0, sizeof(frameFolding_t));

        status = FT_TOOL_FRAME_COMPLEMENT_STATUS_ERROR;

        break;
    }
    }

    return status;
}

ftToolProcessStatus_t FtToolProcess(ftToolFrame_t *ftToolFrame)
{
    ftToolProcessStatus_t status = FT_TOOL_PROCESS_OK;
    frameCommandName_t ftToolCommand = ftToolFrame->command;

    switch (ftToolCommand) {
    case FRAME_COMMAND_NAME_GET_PARAM_NUMBER: {
        ftToolFrame->dataSize = 1;
        ftToolFrame->data[0] = sParamExecCount;

        break;
    }
    case FRAME_COMMAND_NAME_GET_PARAM_DEFINITION: {
        uint8_t paramNumber = ftToolFrame->data[0];
        if (paramNumber < sParamExecCount) {
            ftToolFrame->dataSize = sizeof(ftToolDiagParam_t);
            memcpy(ftToolFrame->data, &sParamExec[paramNumber].diagParam, ftToolFrame->dataSize);
        }
        else {
            status = FT_TOOL_PROCESS_PARAM_OUT_OF_RANGE;
        }
        break;
    }

    case FRAME_COMMAND_NAME_GET_VALUE_BY_NUMBER: {
        uint8_t commandIdx = ftToolFrame->data[0];

        if (commandIdx < sParamExecCount) {
            FtToolParamExec_t *paramExec = &sParamExec[commandIdx];
            ftToolFrame->dataSize = paramExec->diagParam.channels * paramExec->diagParam.fieldSize;

            for (uint8_t channelIdx = 0; channelIdx < paramExec->diagParam.channels; channelIdx++) {
                bool readStatus = paramExec->readHandler(
                    channelIdx, &ftToolFrame->data[paramExec->diagParam.fieldSize * channelIdx],
                    paramExec->diagParam.fieldSize);
                if (readStatus == false) {
                    status = FT_TOOL_PROCESS_READ_RETURN_ERROR;

                    break;
                }
            }
        }
        else {
            status = FT_TOOL_PROCESS_PARAM_OUT_OF_RANGE;
        }

        break;
    }
    case FRAME_COMMAND_NAME_SET_VALUE_BY_NUMBER: {
        uint8_t commandIdx = ftToolFrame->data[0];

        if (commandIdx < sParamExecCount) {
            FtToolParamExec_t *selectedCommand = &sParamExec[commandIdx];
            ftToolFrame->dataSize = selectedCommand->diagParam.channels * selectedCommand->diagParam.fieldSize;

            if (selectedCommand->diagParam.permission == FT_TOOL_DIAG_PARAM_PERMISSION_READ_ONLY) {
                status = FT_TOOL_PROCESS_READ_ONLY_PARAM;

                break;
            }

            for (uint8_t channelIdx = 0; channelIdx < selectedCommand->diagParam.channels; channelIdx++) {
                // +1 becouse data to save begin from data[1] address
                uint8_t dataOffset = (selectedCommand->diagParam.fieldSize * channelIdx) + 1;
                bool writeStatus = selectedCommand->writeHandler(
                    channelIdx, &ftToolFrame->data[dataOffset], selectedCommand->diagParam.fieldSize);

                if (writeStatus == false) {
                    status = FT_TOOL_PROCESS_WRITE_RETURN_ERROR;

                    break;
                }
            }
        }
        else {
            status = FT_TOOL_PROCESS_PARAM_OUT_OF_RANGE;
        }

        break;
    }
    case FRAME_COMMAND_NAME_GET_VALUE_BY_NAME:
    case FRAME_COMMAND_NAME_SET_VALUE_BY_NAME: {
        // these commands are currently not supported.
        status = FT_TOOL_PROCESS_UNSUPORT_CMD;

        break;
    }
    default: {
        status = FT_TOOL_PROCESS_UMKNOWN_CMD;

        break;
    }
    }

    if (status != FT_TOOL_PROCESS_OK) {

        return status;
    }

    ftToolFrame->address = DEFAULT_FRAME_ADDRESS_VALUE;
    ftToolFrame->crc = FrameCrcCalc(ftToolFrame);

    return status;
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool IsDiagParamTypeCorrect(ftToolDiagParamType_t paramType)
{
    bool result = false;

    switch (paramType) {
    case FT_TOOL_DIAG_PARAM_TYPE_STRING:
    case FT_TOOL_DIAG_PARAM_TYPE_BYTE:
    case FT_TOOL_DIAG_PARAM_TYPE_BUTTON:
    case FT_TOOL_DIAG_PARAM_TYPE_UNSIGNED:
    case FT_TOOL_DIAG_PARAM_TYPE_SIGNED: {
        result = true;
        break;
    }
    default:
        break;
    }

    return result;
}

static bool IsFrameCommandCorrect(uint8_t command)
{
    bool result = false;

    switch (command) {
    case FRAME_COMMAND_NAME_SET_VALUE_BY_NUMBER:
    case FRAME_COMMAND_NAME_SET_VALUE_BY_NAME:
    case FRAME_COMMAND_NAME_GET_VALUE_BY_NUMBER:
    case FRAME_COMMAND_NAME_GET_VALUE_BY_NAME:
    case FRAME_COMMAND_NAME_GET_PARAM_NUMBER:
    case FRAME_COMMAND_NAME_GET_PARAM_DEFINITION: {
        result = true;
        break;
    }
    default:
        break;
    }

    return result;
}

static uint8_t FrameCrcCalc(ftToolFrame_t *ftToolFrame)
{
    uint8_t crcRes = 0;

    crcRes ^= ftToolFrame->address;
    crcRes ^= ftToolFrame->id;
    crcRes ^= ftToolFrame->command;
    crcRes ^= ftToolFrame->status;
    crcRes ^= ftToolFrame->dataSize;

    for (uint16_t idx = 0; idx < ftToolFrame->dataSize; idx++) {
        crcRes ^= ftToolFrame->data[idx];
    }

    return crcRes;
}
