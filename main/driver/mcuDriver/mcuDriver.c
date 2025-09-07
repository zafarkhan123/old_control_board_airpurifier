/**
 * @file mcuDriver.c
 *
 * @brief Mcu Driver
 *
 * @dir mcuDriver
 * @brief function used by mcu
 *
 * @author matfio
 * @date 2021.10.28
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "mcuDriver.h"

#include "esp_log.h"
#include "esp_system.h"

#include "setting.h"
#include "scheduler/scheduler.h"


/*****************************************************************************
                          PRIVATE DEFINES / MACROS
*****************************************************************************/

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static const char* TAG = "mcuD";

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

void McuDriverDeviceRestart(void)
{
    ESP_LOGI(TAG, "device restart");

    esp_restart();
}

void McuDriverDeviceSafeRestart(void)
{
    SchedulerSave();
    SettingSave();

    ESP_LOGI(TAG, "device safe restart");
    esp_restart();
}

mcuDriverMacAddress_t McuDriverGetWifiMac(void)
{
    static mcuDriverMacAddress_t wifiMacAddress = {};
    if((wifiMacAddress.part[0] == 0) && (wifiMacAddress.part[1] == 0) && (wifiMacAddress.part[2] == 0) && (wifiMacAddress.part[3] == 0) && (wifiMacAddress.part[4] == 0) && (wifiMacAddress.part[5] == 0)){
        esp_read_mac(wifiMacAddress.part, ESP_MAC_WIFI_STA);
    }
    
    return wifiMacAddress;
}

char* McuDriverGetWifiMacStr(void)
{
    static char macStr[18] = {};

    if(macStr[0] == 0)
    {
        mcuDriverMacAddress_t mac = McuDriverGetWifiMac();

        snprintf(macStr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", mac.part[0], mac.part[1], mac.part[2], mac.part[3], mac.part[4], mac.part[5]);
    }

    return macStr;
}

uint64_t McuDriverGetDeviceId(void)
{
    static uint64_t deviceId = 0;

    if(deviceId == 0){
        mcuDriverMacAddress_t mac = McuDriverGetWifiMac();

        for(uint16_t idx = 0; idx < 5; ++idx)
        {
            deviceId |= mac.part[idx];
            deviceId <<= 8;
        }
        deviceId |= mac.part[5];
    }

    return deviceId;
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/