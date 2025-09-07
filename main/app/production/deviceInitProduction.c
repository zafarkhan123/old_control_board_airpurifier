/**
 * @file deviceInit.c
 *
 * @brief Device initialization source file
 *
 * @dir device
 * @brief Device folder
 *
 * @author matfio
 * @date 2021.08.30
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "deviceInitProduction.h"
#include "device/deviceInit.h"

#include "deviceManagerProduction.h"

#include "gpioIsrDriver/gpioIsrDriver.h"
#include "nvsDriver/nvsDriver.h"
#include "factorySettingsDriver/factorySettingsDriver.h"
#include "adcDriver/adcDriver.h"

#include "wifi/wifi.h"

#include "cloud/iotHubClient.h"
#include "webServer/webServer.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
*****************************************************************************/

#define DEVICEMANAGE_STACK_SIZE (40U * 1024U)

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS
*****************************************************************************/

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Initialize task function
 */
static bool TaskInit(void);

/** @brief Create device manager task
 */
static void CreateDeviceManTask(void);

/*****************************************************************************
                     PRIVATE VARIABLES
*****************************************************************************/

static const char* TAG = "initProd";

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

void DeviceInitProduction(void)
{
    // turn off all logs
    esp_log_level_set("*", ESP_LOG_NONE);

    esp_log_level_set(TAG, ESP_LOG_ERROR);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "DeviceInitProduction");

    TaskInit();
    CreateDeviceManTask();
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool TaskInit(void)
{

    bool res = true;
    ESP_LOGI(TAG, "TaskInit");

    res = GpioIsrDriverInit();
    if(res == true){
        ESP_LOGI(TAG, "GpioIsrDriverInit \t\t\tYES");
    }else{
        ESP_LOGE(TAG, "GpioIsrDriverInit \t\t\tNO");
    }
        
    res = DeviceInitCommonI2cInit();
    if(res == true){
        ESP_LOGI(TAG, "DeviceInitCommonI2cInit \t\tYES");
    }else{
        ESP_LOGE(TAG, "DeviceInitCommonI2cInit \t\tNO");
    }

    res = DeviceInitReadDataFromNvs();
    if(res == true){
        ESP_LOGI(TAG, "DeviceInitReadDataFromNvs \t\tYES");
    }else{
        ESP_LOGE(TAG, "DeviceInitReadDataFromNvs \t\tNO");
    }

    res = true;
    res &= WebServerInit();
    res &= IotHubClientInit();
    res &= WifiInit();
    if(res == true){
        ESP_LOGI(TAG, "Wifi init start \t\t\tYES");
    }else{
        ESP_LOGE(TAG, "Wifi init start \t\t\tNO");
    }

    res = AdcDriverInit();
    if(res == true){
        ESP_LOGI(TAG, "AdcDriverInit \t\t\tYES");
    }else{
        ESP_LOGE(TAG, "AdcDriverInit \t\t\tNO");
    }

    res = DeviceInitCommonSpiInit();
    if(res == true){
        ESP_LOGI(TAG, "DeviceInitCommonSpiInit \t\tYES");
    }else{
        ESP_LOGE(TAG, "DeviceInitCommonSpiInit \t\tNO");
    }

    if(res == false){
        ESP_LOGE(TAG, "something went wrong in init stage");
    }

    return res;
}

static void CreateDeviceManTask(void)
{
    BaseType_t res = pdFAIL;

    res = xTaskCreate(DeviceManagerProductionMainLoop, "DevManProductionTask", DEVICEMANAGE_STACK_SIZE, NULL, 3U, NULL);
    assert(res == pdPASS);

    ESP_LOGI(TAG, "DevManTask created");
}