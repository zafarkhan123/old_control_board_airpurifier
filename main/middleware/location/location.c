/**
 * @file location.c
 *
 * @brief Location middleware source file
 *
 * @dir location
 * @brief Location middleware folder
 *
 * @author matfio
 * @date 2021.11.24
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "location.h"

#include <esp_log.h>

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "nvsDriver/nvsDriver.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define MUTEX_TIMEOUT_MS (1U * 1000U)
#define NVS_KAY_NAME ("Location")

#define DAFAULT_LOCATION_ADDRESS ("N/A")
#define DAFAULT_LOCATION_ROOM ("N/A")

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static const char *TAG = "location";

static SemaphoreHandle_t sLocationMutex;
static Location_t sLocation;

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool LocationInit(void)
{
    sLocationMutex = xSemaphoreCreateMutex();
    if (sLocationMutex == NULL) {
        return false;
    }

    LocationLoad();

    return true;
}

bool LocationLoad(void)
{
    if(xSemaphoreTake(sLocationMutex, MUTEX_TIMEOUT_MS) == pdTRUE){
        Location_t loadLocation = {};
        uint16_t loadDataLen = sizeof(Location_t);

        bool nvsRes = NvsDriverLoad(NVS_KAY_NAME, &loadLocation, &loadDataLen);
        ESP_LOGI(TAG, "load data len %d", loadDataLen);

        if(nvsRes == true){
            ESP_LOGI(TAG, "load location from nvs");
            memcpy(&sLocation, &loadLocation, sizeof(Location_t));
        }
        else{
            ESP_LOGI(TAG, "save default location nvs");

            memcpy(&sLocation.address, DAFAULT_LOCATION_ADDRESS, sizeof(DAFAULT_LOCATION_ADDRESS));
            memcpy(&sLocation.room, DAFAULT_LOCATION_ROOM, sizeof(DAFAULT_LOCATION_ROOM));

            nvsRes = NvsDriverSave(NVS_KAY_NAME, &sLocation, sizeof(Location_t));
        }

        LocationPrintf(&sLocation);
        xSemaphoreGive(sLocationMutex);

        return nvsRes;
    }

    return false;
}

void LocationPrintf(Location_t* location)
{
    ESP_LOGI(TAG, "Location size %d", sizeof(Location_t));

    ESP_LOGI(TAG, "Address %s", location->address);

    ESP_LOGI(TAG, "Room %s", location->room);
}

bool LocationGet(Location_t* location)
{
    if (xSemaphoreTake(sLocationMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        memcpy(location, &sLocation, sizeof(Location_t));

        xSemaphoreGive(sLocationMutex);
        return true;
    }
    
    return false;
}

bool LocationSet(Location_t* location)
{
    if (xSemaphoreTake(sLocationMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        memcpy(&sLocation, location, sizeof(Location_t));

        xSemaphoreGive(sLocationMutex);
        return true;
    }
    
    return false;
}

bool LocationSave(void)
{
    if (xSemaphoreTake(sLocationMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        bool res = NvsDriverSave(NVS_KAY_NAME, &sLocation, sizeof(Location_t));

        xSemaphoreGive(sLocationMutex);
        return res;
    }

    return false;
}

float LocationGetUtcOffset(void)
{
    float offset = 0.0f;

    if (xSemaphoreTake(sLocationMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        offset = sLocation.utcTimeHoursOffset;
        xSemaphoreGive(sLocationMutex);
    }  
    return offset;
}

bool LocationSetUtcOffset(float offset)
{
    if (xSemaphoreTake(sLocationMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        sLocation.utcTimeHoursOffset = offset;
        xSemaphoreGive(sLocationMutex);
        return true;
    } 
    return false;
}