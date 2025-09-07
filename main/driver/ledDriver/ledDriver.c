/**
 * @file ledDriver.c
 *
 * @brief Rgb led driver source file
 *
 * @dir ledDriver
 * @brief Led driver folder
 * Idea of ​​coding from 
 * https://forbot.pl/forum/topic/17328-diody-ws2812b-sterowane-przez-uart/
 *
 * @author matfio
 * @date 2021.08.20
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "config.h"

#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "ledDriver.h"
#include "driver/gpio.h"

#include "gpioExpanderDriver/gpioExpanderDriver.h"
#include "factorySettingsDriver/factorySettingsDriver.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define MUTEX_TIMEOUT_MS (1000U)
#define DELAY_MS (50U)

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static rgb_t sColors[LED_COLOR_COUNT] = {
    [LED_COLOR_OFF] = { .r = 0x00, .g = 0x00, .b = 0x00 },
    [LED_COLOR_WHITE] = { .r = 0xa6, .g = 0xa6, .b = 0xa6 },
    [LED_COLOR_RED] = { .r = 0xff, .g = 0x00, .b = 0x00 },
    [LED_COLOR_GREEN] = { .r = 0x00, .g = 0xff, .b = 0x00 },
    [LED_COLOR_BLUE] = { .r = 0x00, .g = 0x00, .b = 0xff },
    [LED_COLOR_ORANGE] = { .r = 0xed, .g = 0x70, .b = 0x14 },
    [LED_COLOR_LOGO] = { .r = 0xff, .g = 0xff, .b = 0xff },
};

static LedDriverColor_t sActualLedColor[LED_NAME_COUNT];

static const char *TAG = "ledD";

static SemaphoreHandle_t sLedMutex;

static bool sColorChanged;

static led_strip_t sStrip = {
    .type = LED_STRIP_WS2812,
    .length = LED_NAME_COUNT,
    .gpio = CFG_RGB_LED_DATA_GPIO_PIN,
    .buf = NULL,
};

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Turn on the power for the leds
 *  @return return true if success
 */
static bool LedPowerOn(void);

/** @brief Set logo color from factory setting
 *  @return return true if success
 */
static bool SetLogoColorFromFactorySetting(void);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool LedDriverInit(void)
{
    bool res = true;

    led_strip_install();

    res &= LedPowerOn();
    res &= SetLogoColorFromFactorySetting();
    
    esp_err_t err = led_strip_init(&sStrip);
    if(err != ESP_OK){
        return false;
    }

    sLedMutex = xSemaphoreCreateMutex();
    if (sLedMutex == NULL) {
        return false;
    }

    if (xSemaphoreTake(sLedMutex, MUTEX_TIMEOUT_MS) == pdTRUE){
        vTaskDelay(DELAY_MS);
        
        for(uint16_t idx = 0; idx < LED_NAME_COUNT; ++idx)
        {
            LedDriverColor_t actualLedColor = sActualLedColor[idx];
            led_strip_set_pixel(&sStrip, idx, sColors[actualLedColor]);
        }

        led_strip_flush(&sStrip);

        vTaskDelay(DELAY_MS);
        
        xSemaphoreGive(sLedMutex);
    }


    ESP_LOGI(TAG, "initialize");

    vTaskDelay(DELAY_MS);

    return res;
}

bool LedDriverDeinit(void)
{
    led_strip_free(&sStrip);
    
    return GpioExpanderDriverLedOff();
}

bool LedDriverSetColor(LedDriverName_t ledName, LedDriverColor_t color)
{
    if((ledName >= LED_NAME_COUNT) || (color >= LED_COLOR_COUNT))
    {
        return false;
    }
    
    if(sActualLedColor[ledName] == color)
    {
        return true;
    }

    if (xSemaphoreTake(sLedMutex, MUTEX_TIMEOUT_MS) == pdTRUE){

        sActualLedColor[ledName] = color;
        rgb_t newColor = sColors[color];
        
        led_strip_set_pixel(&sStrip, ledName, newColor);

        ESP_LOGI(TAG, "led num %d, r %X, g %X, b %X", ledName, newColor.r, newColor.g, newColor.b);

        sColorChanged = true;
        
        xSemaphoreGive(sLedMutex);
    }
    
    return true;
}

bool LedDriverChangeColor(void)
{
    if(sColorChanged == false)
    {
        return true;
    }

    if (xSemaphoreTake(sLedMutex, MUTEX_TIMEOUT_MS) == pdTRUE){
        vTaskDelay(DELAY_MS);
        
        led_strip_flush(&sStrip);

        vTaskDelay(DELAY_MS);
        sColorChanged = false;

        xSemaphoreGive(sLedMutex);
    }

    ESP_LOGI(TAG, "change color");

    return true;
}

rgb_t LedDriverGetColorComponents(LedDriverColor_t colorName)
{
    if(colorName >= LED_COLOR_COUNT){
        rgb_t rgb = {};
        return rgb;
    }

    return sColors[colorName];
}

void LedDriverSetColorComponents(LedDriverColor_t colorName, rgb_t newColorComp)
{
    if(colorName >= LED_COLOR_COUNT){
        return;
    }

    sColors[colorName] = newColorComp;
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool LedPowerOn(void)
{
    //Turn on power
    return GpioExpanderDriverLedOn();
}

static bool SetLogoColorFromFactorySetting(void)
{
    bool res = true;

    rgb_t logo = {};
    uint32_t readColor = CFG_TOUCH_DEFAULT_LOGO_COLOR;

    res &= FactorySettingsGetServiceParam(FACTORY_SETTING_SERVICE_LOGO_LED_COLOR, &readColor);
    if(res == false){
        ESP_LOGW(TAG, "the color of the logo cannot be read");
        return false;
    }

    uint8_t colorComponent = (uint8_t)(readColor & 0x000000ff);
    logo.b = colorComponent;

    readColor = readColor >> 8;
    colorComponent = (uint8_t)(readColor & 0x000000ff);
    logo.g = colorComponent;

    readColor = readColor >> 8;
    colorComponent = (uint8_t)(readColor & 0x000000ff);
    logo.r = colorComponent;

    sColors[LED_COLOR_LOGO].r = logo.r;
    ESP_LOGI(TAG, "LOGO color r %X", logo.r);
    sColors[LED_COLOR_LOGO].g = logo.g;
    ESP_LOGI(TAG, "LOGO color g %X", logo.g);
    sColors[LED_COLOR_LOGO].b = logo.b;
    ESP_LOGI(TAG, "LOGO color b %X", logo.b);

    return res;
}