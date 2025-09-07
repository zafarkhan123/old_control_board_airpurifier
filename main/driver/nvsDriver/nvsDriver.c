/**
 * @file nvsDriver.c
 *
 * @brief Non-volatile storage driver source file
 *
 * @dir nvsDriver
 * @brief NVS driver folder
 *
 * @author matfio
 * @date 2021.09.07
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "nvsDriver.h"
#include "config.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define NVS_STORAGE_NAMESPACE "storage"

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool NvsDriverInit(void)
{   
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if(err == ESP_OK){
        return true;
    }
    
    return false;
}

bool NvsDriverSave(char *key, void *outValue, uint16_t valueLen)
{
    nvs_handle_t nvsHandle;

    // Open
    esp_err_t err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK){
        return false;
    }

    // Write value
    err = nvs_set_blob(nvsHandle, key, outValue, valueLen);
    if (err != ESP_OK){
        nvs_close(nvsHandle);
        return false;
    }
    bool res = true;
    // Commit
    err = nvs_commit(nvsHandle);
    if(err != ESP_OK){
        res = false;
    }

    // Close
    nvs_close(nvsHandle);

    return res;
}

bool NvsDriverLoad(char *key, void *inValue, uint16_t* valueLen)
{
    nvs_handle_t nvsHandle;

    // Open
    esp_err_t err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK){
        return false;
    }

    // Read value
    bool res = true;
    nvs_get_blob(nvsHandle, key, NULL, (size_t*)valueLen);

    err = nvs_get_blob(nvsHandle, key, inValue, (size_t*)valueLen);
    if(err != ESP_OK){
        res = false;
    }

    // Close
    nvs_close(nvsHandle);

    return res;
}