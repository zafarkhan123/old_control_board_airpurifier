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

#include "config.h"
#include "setting.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"

#include "driver/i2c.h"
#include "hal/i2c_types.h"

#include "deviceInit.h"
#include "deviceManager.h"
#include "cloud/iotHubClient.h"
#include "webServer/webServer.h"

#include "fan/fan.h"
#include "led/led.h"
#include "touch/touch.h"
#include "scheduler/scheduler.h"
#include "location/location.h"
#include "ftTool/ftTool.h"
#include "wifi/wifi.h"
#include "uvLamp/uvLamp.h"

#include "gpioIsrDriver/gpioIsrDriver.h"
#include "adcDriver/adcDriver.h"
#include "timerDriver/timerDriver.h"
#include "nvsDriver/nvsDriver.h"
#include "factorySettingsDriver/factorySettingsDriver.h"
#include "rtcDriver/rtcDriver.h"
#include "ethernetDriver/ethernetDriver.h"
#include "gpioExpanderDriver/gpioExpanderDriver.h"
#include "externalFlashDriver/externalFlashDriver.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
*****************************************************************************/

#define TASK_INIT_NAME_LEN (16U)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define DEVINIT_TASK_PRIORITY_NORMAL (3U)
#define DEVINIT_TASK_PRIORITY_HIGH (4U)

#define DEVICEMANAGE_STACK_SIZE (5U * 1024U)
#define FT_TOOL_STACK_SIZE (5U * 1024U)

#define CLOUD_STACK_SIZE (8U * 1024U)

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS
*****************************************************************************/

typedef enum {
    CRITICAL, 
    OPTIONAL, 
    DISABLED 
} taskStatus_t;

typedef bool (*taskInitProcedure_t)(void);

typedef struct {
    char name[TASK_INIT_NAME_LEN];
    taskInitProcedure_t initProcedure;
    taskStatus_t status;
} __attribute__ ((packed)) taskInit_t;

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Initialize task function
 */
static void TaskInit(void);

/** @brief Create device manager task
 */
static void CreateDeviceManTask(void);

/** @brief Create ft tool task
 */
static void CreateFtToolTask(void);

/** @brief Create cloud task
 */
static void CreateCloudTask(void);

/** @brief Start webserver task
 */
static void StartWebserverTask(void);

/*****************************************************************************
                     PRIVATE VARIABLES
*****************************************************************************/

static const char* TAG = "devInit";

// the order in the list does matter
static const taskInit_t sTaskInitList[] = {
    { "Gpio irq", GpioIsrDriverInit, CRITICAL },
    { "Comm I2C", DeviceInitCommonI2cInit, CRITICAL },
    { "Gpio expa", GpioExpanderDriverInit, CRITICAL }, 
    { "NVS Init", DeviceInitReadDataFromNvs, CRITICAL },
    { "IotHub Init", IotHubClientInit, CRITICAL },
    { "Rtc init", RtcDriverInit, CRITICAL },
    { "Webserver Init", WebServerInit, CRITICAL },
    { "Adc init", AdcDriverInit, CRITICAL },
    { "Timer init", TimerDriverInit, CRITICAL },
    { "UvLamp Init", UvLampInit, CRITICAL },
    { "Fan Init", FanInit, CRITICAL },
    { "Touch init", TouchInit, CRITICAL },
    { "Wifi Init", WifiInit, CRITICAL },
    { "Comm SPI", DeviceInitCommonSpiInit, CRITICAL },
    { "Ext Flash", ExternalFlashDriverInit, CRITICAL },
    { "Ethernet", ethernetDriverInit, CRITICAL},
};

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

void DeviceInit(void)
{
    TaskInit();

    CreateDeviceManTask();
    CreateCloudTask();
    CreateFtToolTask();
    StartWebserverTask();
}

bool DeviceInitCommonI2cInit(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CFG_I2C_DATA_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = CFG_I2C_CLK_PIN,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = CFG_I2C_FREQ_HZ,
        // .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
    };

    esp_err_t err = i2c_param_config(CFG_I2C_PORT_NUMBER, &conf);
    if (err != ESP_OK) {
        return false;
    }

    err = i2c_driver_install(CFG_I2C_PORT_NUMBER, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        return false;
    }

    return true;
}

bool DeviceInitCommonSpiInit(void)
{
    const spi_bus_config_t buscfg = {
        .miso_io_num = CFG_SPI_MISO_GPIO,
        .mosi_io_num = CFG_SPI_MOSI_GPIO,
        .sclk_io_num = CFG_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    if(spi_bus_initialize(CFG_SPI_HOST_NUMBER, &buscfg, 1) != ESP_OK){
        return false;
    }

    return true;
}

bool DeviceInitReadDataFromNvs(void)
{
    bool res = true;
    
    res &= NvsDriverInit();
    res &= FactorySettingsDriverInit();
    res &= SettingInit();
    res &= SchedulerInit();
    res &= LocationInit();
    res &= WifiSettingInit();

    return res;
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static void TaskInit(void)
{
    ESP_LOGI(TAG, "TaskInit");

    for (uint8_t i = 0; i < ARRAY_SIZE(sTaskInitList); i++) {
        assert(sTaskInitList[i].initProcedure);

        ESP_LOGI(TAG, "%s", sTaskInitList[i].name);

        if (sTaskInitList[i].initProcedure() == false) {
            ESP_LOGE(TAG, "Late init procedure failed: %s\n", sTaskInitList[i].name);

            if (sTaskInitList[i].status == CRITICAL) {
                assert(false);
            }
        }
    }
}

static void CreateDeviceManTask(void)
{
    BaseType_t res = pdFAIL;
    // starting the thread in which the diode is initialized (RMT) must be on another core than wifi
    res = xTaskCreatePinnedToCore(DeviceManagerMainLoop, "DevManTask", DEVICEMANAGE_STACK_SIZE, NULL, DEVINIT_TASK_PRIORITY_NORMAL, NULL, 1U);
    assert(res == pdPASS);

    ESP_LOGI(TAG, "DevManTask created");
}

static void CreateFtToolTask(void)
{
    #if CFG_FT_TOOL_ENABLE
    BaseType_t res = pdFAIL;

    res = xTaskCreate(FtToolMainLoop, "FtToolTask", FT_TOOL_STACK_SIZE, NULL, DEVINIT_TASK_PRIORITY_NORMAL, NULL);
    assert(res == pdPASS);

    ESP_LOGI(TAG, "FtToolTask created");
    #endif
}

static void CreateCloudTask(void)
{   
    #if CFG_HTTP_CLIENT_ENABLE
    BaseType_t res = pdFAIL;

    res = xTaskCreatePinnedToCore(IotHubClientMainLoop, "CloudTask", CLOUD_STACK_SIZE, NULL, DEVINIT_TASK_PRIORITY_NORMAL, NULL, 0U);
    assert(res == pdPASS);

    ESP_LOGI(TAG, "CloudTask created");
    #else
    ESP_LOGW(TAG, "CloudTask will not be created");
    #endif
}

static void StartWebserverTask(void)
{
    iotHubClientStatus_t setting = {};
    IotHubClientGetSetting(&setting);

    if(setting.isConnectedLeastOnce == false){
        ESP_LOGI(TAG, "WebServer Start");

        WebServerStart();
    }
    else{
        ESP_LOGW(TAG, "WebServer will not be started");
    }

}