/**
 * @file ftTool.c
 *
 * @brief FT Tool middleware source file
 *
 * @dir ftTool
 * @brief ftTool middleware folder
 *
 * @author matfio
 * @date 2021.10.19
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "setting.h"

#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ftTool.h"
#include "ftToolUser.h"
#include "utils/circBuff/circBuff.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "ledDriver/ledDriver.h"
#include "ftToolDriver/ftToolDriver.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define UART_PORT_NUMBER (UART_NUM_2)
#define UART_BAUD_RATE (115200U)

#define UART_BUFFER_SIZE (256U)
#define INPUT_BUFFER_SIZE (32U)
#define OUTPUT_BUFFER_SIZE (128U)

#define TASK_DELAY_MS (10U)
#define READ_UART_TIMEOUT_MS (100U)

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static const char *TAG = "FtTool";

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Initialize Uart
 *  @return return true if success
 */
static bool UartInit(void);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

void FtToolMainLoop(void *argument)
{
    const FtToolParamExec_t ftToolParamExecTable[] = {
        {{ "OFF", FT_TOOL_DIAG_PARAM_PERMISSION_READ_WRITE, sizeof(rgb_t), 1, FT_TOOL_DIAG_PARAM_TYPE_UNSIGNED, 10, 0, "rgb", "Color Off" },
            FtToolUserReadColorOffComponent, FtToolUserWriteColorOffComponent },
        {{ "WHI", FT_TOOL_DIAG_PARAM_PERMISSION_READ_WRITE, sizeof(rgb_t), 1, FT_TOOL_DIAG_PARAM_TYPE_UNSIGNED, 10, 0, "rgb", "Color White" },
            FtToolUserReadColorWhiteComponent, FtToolUserWriteColorWhiteComponent },
        {{ "RED", FT_TOOL_DIAG_PARAM_PERMISSION_READ_WRITE, sizeof(rgb_t), 1, FT_TOOL_DIAG_PARAM_TYPE_UNSIGNED, 10, 0, "rgb", "Color Red" },
            FtToolUserReadColorRedComponent, FtToolUserWriteColorRedComponent },
        {{ "GRE", FT_TOOL_DIAG_PARAM_PERMISSION_READ_WRITE, sizeof(rgb_t), 1, FT_TOOL_DIAG_PARAM_TYPE_UNSIGNED, 10, 0, "rgb", "Color Green" },
            FtToolUserReadColorGreenComponent, FtToolUserWriteColorGreenComponent },
        {{ "BLU", FT_TOOL_DIAG_PARAM_PERMISSION_READ_WRITE, sizeof(rgb_t), 1, FT_TOOL_DIAG_PARAM_TYPE_UNSIGNED, 10, 0, "rgb", "Color Blue" },
            FtToolUserReadColorBlueComponent, FtToolUserWriteColorBlueComponent },
        {{ "ORA", FT_TOOL_DIAG_PARAM_PERMISSION_READ_WRITE, sizeof(rgb_t), 1, FT_TOOL_DIAG_PARAM_TYPE_UNSIGNED, 10, 0, "rgb", "Color Orange" },
            FtToolUserReadColorOrangeComponent, FtToolUserWriteColorOrangeComponent },
        {{ "FAN", FT_TOOL_DIAG_PARAM_PERMISSION_READ_WRITE, FAN_LEVEL_COUNT, 2, FT_TOOL_DIAG_PARAM_TYPE_UNSIGNED, 10, 0, "bit", "Fan Level" },
            FtToolUserReadPwmFanLevel, FtToolUserWritePwmFanLevel },
        {{ "TAC", FT_TOOL_DIAG_PARAM_PERMISSION_READ_ONLY, 1, 2, FT_TOOL_DIAG_PARAM_TYPE_UNSIGNED, 10, 0, "RPS", "Tacho speed" },
            FtToolUserReadTachoSpeed, NULL },
        {{ "BAV", FT_TOOL_DIAG_PARAM_PERMISSION_READ_ONLY, 2, 4, FT_TOOL_DIAG_PARAM_TYPE_UNSIGNED, 10, 0, "mV", "Uv lamp" },
            FtToolUserReadUvLampVoltage, NULL },
    };

    uint8_t singleChar = 0;
    ftToolFrame_t ftFrame = {};
    uint8_t outBuffer[OUTPUT_BUFFER_SIZE] = {};

    uint8_t tableElementNumber = (sizeof(ftToolParamExecTable) / sizeof(FtToolParamExec_t));
    ESP_LOGI(TAG, "number of param in array %d", tableElementNumber);

    bool res = FtToolInit(ftToolParamExecTable, tableElementNumber);
    ESP_LOGI(TAG, "Check ft tool command array %d", res);
    assert(res);

    res = UartInit();
    ESP_LOGI(TAG, "Uart init status %d", res);
    assert(res);
    
    for (;;) {
        int readBytesNumber = uart_read_bytes(UART_PORT_NUMBER, &singleChar, sizeof(singleChar), (READ_UART_TIMEOUT_MS / portTICK_RATE_MS));
        if(readBytesNumber > 0)
        { 
            ftToolFrameComplementStatus_t complettingStatus = FtToolFrameComplement(singleChar, &ftFrame);
            if (complettingStatus == FT_TOOL_FRAME_COMPLEMENT_STATUS_CORRECT) {
  
                ftToolProcessStatus_t status = FtToolProcess(&ftFrame);
                if (status == FT_TOOL_PROCESS_OK) {
                    uint16_t outDataSize = FtToolPrepareOutputBuffer(&ftFrame, outBuffer);
                    uart_write_bytes(UART_PORT_NUMBER, outBuffer, outDataSize);
                }
            }
        }
        vTaskDelay(TASK_DELAY_MS);
    }
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool UartInit(void)
{
    /* Configure parameters of an UART driver */
    uart_config_t uartConfig = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    esp_err_t res = uart_driver_install(UART_PORT_NUMBER, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 0, NULL, 0);
    if(res != ESP_OK){
        return false;
    }

    res = uart_param_config(UART_PORT_NUMBER, &uartConfig);
    if(res != ESP_OK){
        return false;
    }

    res = uart_set_pin(UART_PORT_NUMBER, CFG_FT_TOOL_TXD_GPIO_PIN, CFG_FT_TOOL_RXD_GPIO_PIN, -1, -1);
    if(res != ESP_OK){
        return false;
    }

    return true;
}