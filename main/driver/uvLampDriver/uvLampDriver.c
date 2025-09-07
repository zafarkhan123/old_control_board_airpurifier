/**
 * @file uvLampDriver.c
 *
 * @brief UV Lamp driver source file
 *
 * @dir uvLampDriver
 * @brief uvLampDriver driver folder
 *
 * @author matfio
 * @date 2021.08.23
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "config.h"

#include "esp_log.h"

#include "uvLampDriver.h"

#include "driver/gpio.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define GPIO_OUTPUT_UV_LAMP_PIN_SEL  ((1ULL << CFG_UV_LAMP_BALLAST_1_ON_OFF_GPIO_PIN) | (1ULL << CFG_UV_LAMP_BALLAST_2_ON_OFF_GPIO_PIN))

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static const char* TAG = "uvLampD";

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/
/** @brief Set uv lamp pin as output
 */
static bool UvLampOutputPinInit(void);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool UvLampDriverInit(void)
{
    bool res = true;

    res &= UvLampOutputPinInit();

    return res;
}

bool UvLampDriverSetLevel(uvLampNumber_t lampNumber, uint32_t level)
{
    if(lampNumber >= UV_LAMP_COUNT)
    {
        return false;
    }

    esp_err_t result = ESP_OK;

    if(lampNumber == UV_LAMP_1)
    {
        result = gpio_set_level(CFG_UV_LAMP_BALLAST_1_ON_OFF_GPIO_PIN, level);
    }
    else if(lampNumber == UV_LAMP_2)
    {
        result = gpio_set_level(CFG_UV_LAMP_BALLAST_2_ON_OFF_GPIO_PIN, level);
    }

    ESP_LOGI(TAG, "lamp %d is %d", (lampNumber + 1), level);

    if(result == ESP_OK)
    {
        return true;
    }

    return false;
}

static bool UvLampOutputPinInit(void)
{
    gpio_config_t io_conf;

    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;

    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;

    //bit mask of the pins
    io_conf.pin_bit_mask = GPIO_OUTPUT_UV_LAMP_PIN_SEL;

    //disable pull-down mode
    io_conf.pull_down_en = 0;

    //disable pull-up mode
    io_conf.pull_up_en = 0;

    //configure GPIO with the given settings
    gpio_config(&io_conf);

    gpio_set_level(CFG_UV_LAMP_BALLAST_1_ON_OFF_GPIO_PIN, 0);
    gpio_set_level(CFG_UV_LAMP_BALLAST_2_ON_OFF_GPIO_PIN, 0);

    return true;
}