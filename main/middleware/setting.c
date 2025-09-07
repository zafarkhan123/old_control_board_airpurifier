/**
 * @file setting.c
 *
 * @brief setting middleware source file
 *
 * @dir setting
 * @brief middleware folder
 *
 * @author matfio
 * @date 2021.09.02
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "setting.h"
#include "config.h"

#include "timerDriver/timerDriver.h"

#include <esp_log.h>

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "nvsDriver/nvsDriver.h"
#include "timeDriver/timeDriver.h"
#include "rtcDriver/rtcDriver.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define SETTING_MUTEX_TIMEOUT_MS (5U * 1000U)
#define NVS_KAY_NAME ("setting")

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static const char *TAG = "setting";

static bool sIsSaveError;

static SemaphoreHandle_t sSettingMutex;
static SettingDevice_t sSettingDevice;

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool SettingInit(void)
{
    #if CFG_TEST_TYPE == CFG_TEST_ALARM
        #warning "TEST MODE set memory alarm flag"
        sIsSaveError = true;
    #endif

    sSettingMutex = xSemaphoreCreateMutex();
    if (sSettingMutex == NULL) {
        return false;
    }

    SettingLoad();
    
    return true;
}

bool SettingLoad(void)
{
    if (xSemaphoreTake(sSettingMutex, SETTING_MUTEX_TIMEOUT_MS) == pdTRUE) {
        SettingRestore_t loadSetting = {};
        uint16_t loadDataLen = sizeof(SettingRestore_t);
    
        bool nvsRes = NvsDriverLoad(NVS_KAY_NAME, &loadSetting, &loadDataLen);
        ESP_LOGI(TAG, "load data len %d", loadDataLen);

        if(nvsRes == true){
            if(loadDataLen == sizeof(SettingRestore_t)){
                // Load setting OK
                ESP_LOGI(TAG, "load setting from nvs");
                memcpy(&sSettingDevice.restore, &loadSetting, sizeof(SettingRestore_t));
            }else{
                ESP_LOGI(TAG, "read mismatch size");
                NvsDriverSave(NVS_KAY_NAME, &sSettingDevice.restore, sizeof(SettingRestore_t));
            }
        }else{
            ESP_LOGI(TAG, "write default value");
            nvsRes = NvsDriverSave(NVS_KAY_NAME, &sSettingDevice.restore, sizeof(SettingRestore_t));
        }

        xSemaphoreGive(sSettingMutex);
        return nvsRes;
    }
    return false;
}

bool SettingSave(void)
{
    if (xSemaphoreTake(sSettingMutex, SETTING_MUTEX_TIMEOUT_MS) == pdTRUE) {
        if(RtcDriverIsError() == false){ // to not lose the error after some time of operation
            // Saves the current time to check if the rtc battery is still functional after power back
            sSettingDevice.restore.saveTimestamp = TimeDriverGetUTCUnixTime();
        }

        bool res = NvsDriverSave(NVS_KAY_NAME, &sSettingDevice.restore, sizeof(SettingRestore_t));
        if(res == false){
            sIsSaveError = true;
        }

        xSemaphoreGive(sSettingMutex);
        return res;
    }
    return false;
}

bool SettingGet(SettingDevice_t* setting)
{
    if (xSemaphoreTake(sSettingMutex, SETTING_MUTEX_TIMEOUT_MS) == pdTRUE) {
        memcpy(setting, &sSettingDevice, sizeof(SettingDevice_t));
        xSemaphoreGive(sSettingMutex);
        return true;
    }
    return false;
}

bool SettingSet(SettingDevice_t* setting)
{
    if (xSemaphoreTake(sSettingMutex, SETTING_MUTEX_TIMEOUT_MS) == pdTRUE) {
        memcpy(&sSettingDevice, setting, sizeof(SettingDevice_t));
        xSemaphoreGive(sSettingMutex);
        return true;
    }
    return false;
}

bool SettingUpdateDeviceStatus(SettingDevice_t* setting)
{
    if (xSemaphoreTake(sSettingMutex, SETTING_MUTEX_TIMEOUT_MS) == pdTRUE) {

        if(memcmp(&sSettingDevice.restore.deviceStatus, &setting->restore.deviceStatus, sizeof(SettingDeviceStatust_t)) != 0){
            memcpy(&sSettingDevice.restore.deviceStatus, &setting->restore.deviceStatus, sizeof(SettingDeviceStatust_t));
        }
        
        xSemaphoreGive(sSettingMutex);
        return true;
    }
    return false;   
}

bool SettingUpdateDeviceMode(SettingDevice_t* setting)
{
    if (xSemaphoreTake(sSettingMutex, SETTING_MUTEX_TIMEOUT_MS) == pdTRUE) {
        sSettingDevice.restore.deviceMode = setting->restore.deviceMode;
        
        xSemaphoreGive(sSettingMutex);
        return true;
    }
    return false;   
}

bool SettingUpdateTimers(SettingDevice_t* timer)
{
    if (xSemaphoreTake(sSettingMutex, SETTING_MUTEX_TIMEOUT_MS) == pdTRUE) {

        if(memcmp(&sSettingDevice.restore.liveTime, timer->restore.liveTime, (TIMER_NAME_COUNTER * sizeof(uint64_t))) != 0){
            memcpy(&sSettingDevice.restore.liveTime, timer->restore.liveTime, (TIMER_NAME_COUNTER * sizeof(uint64_t)));
        }
        
        xSemaphoreGive(sSettingMutex);
        return true;
    }
    return false;   
}

bool SettingUpdateTouchScreen(const bool lock)
{
    if (xSemaphoreTake(sSettingMutex, SETTING_MUTEX_TIMEOUT_MS) == pdTRUE) {
        sSettingDevice.restore.touchLock = lock;

        xSemaphoreGive(sSettingMutex);
        return true;
    }
    return false;   
}

bool SettingIsError(void)
{
    return sIsSaveError;
}