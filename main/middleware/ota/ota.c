/**
 * @file ota.c
 *
 * @brief ota middleware source file
 *
 * @dir ota
 * @brief Over The Air Updates
 *
 * @author matfio
 * @date 2021.11.26
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "config.h"
#include "ota.h"
#include "esp_crc.h"

#include <string.h>
#include <mbedtls/md.h>

#include "esp_ota_ops.h"

#include "esp_http_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "mcuDriver/mcuDriver.h"
#include "timeDriver/timeDriver.h"
#include "factorySettingsDriver/factorySettingsDriver.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define FIRMWARE_VERSION_STRING_SIZE (32U)
#define DATA_BUFFOR_SIZE (512U)

#define TASK_STACK_SIZE (8U * 1024U)
#define TASK_DELAY_MS (250U)

#define UPDATE_DOWNLOAD_TIMEOUT (25U * 60U * 1000U)        // 25 minute

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static OtaStatus_t sOtaStatus = OTA_NOTHING_TO_DO;

static TaskHandle_t sTaskHandle;
static esp_http_client_handle_t sFileClientHandler;

static const char* TAG = "Ota";

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Close open http client
 */
static void CloseHttpClient(void);

/** @brief Delay ota task
 */
static void DelayTask(void);

/** @brief Task main loop
 *  @param argument [in] pointer parameter send to task
 */
static void OtaMainLoop(void *argument);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

OtaStatus_t OtaGetStatus(void)
{
    return sOtaStatus;
}

void  OtaMarkValid(void)
{
    esp_ota_mark_app_valid_cancel_rollback();
}

void  OtaMarkInvalid(void)
{
    esp_ota_mark_app_invalid_rollback_and_reboot();
}

bool OtaIsUpdateNeeded(Ota_t* updateCandidate)
{
    if(updateCandidate->isAvailable == false){
        return false;
    }

    OtaFirmwareVersion_t actualVersion = {};

    bool res = OtaGetFirmwareVersion(&actualVersion);
    bool cmpStatus = (memcmp(updateCandidate, &actualVersion, sizeof(OtaFirmwareVersion_t)) == 0);

    return (res & cmpStatus);
}

void OtaCreateTask(Ota_t* updateCandidate)
{
    bool isRunning = false;

    if(sTaskHandle == NULL){
        isRunning = false;
    }else{
        eTaskState state = eTaskGetState(sTaskHandle);
        if(state == eRunning){
            isRunning = true;
        }
        else{
            isRunning = false;
        }
    }

    if(isRunning == true){
        ESP_LOGW(TAG, "OtaTask is running");
        return;
    }

    OtaFirmwareVersion_t actualVersion = {};
    bool getVersionRes = OtaGetFirmwareVersion(&actualVersion);
    if(getVersionRes == false){
        ESP_LOGE(TAG, "read version is impossible");
        return;
    }

    ESP_LOGI(TAG, "actual version %u.%u.%u", actualVersion.major, actualVersion.minor, actualVersion.subMinor);
    ESP_LOGI(TAG, "new version %u.%u.%u", updateCandidate->version.major, updateCandidate->version.minor, updateCandidate->version.subMinor);

    int cmpRes = memcmp(&actualVersion, &updateCandidate->version, sizeof(OtaFirmwareVersion_t));
    if(cmpRes == 0){
        ESP_LOGW(TAG, "update to the same version");
        return;
    }
    
    static Ota_t ota = {};
    memcpy(&ota, updateCandidate, sizeof(Ota_t));

    BaseType_t res = pdFAIL;

    res = xTaskCreate(OtaMainLoop, "OtaTask", TASK_STACK_SIZE, &ota, 2U, &sTaskHandle);
    assert(res == pdPASS);

    ESP_LOGI(TAG, "OtaTask created");
}

bool OtaGetFirmwareVersion(OtaFirmwareVersion_t *version)
{
    char otaVersionString[OTA_NEW_VERSION_STRING_LEN] = {};

    const esp_app_desc_t *app_desc = esp_ota_get_app_description();

    memcpy(otaVersionString, app_desc->version, OTA_NEW_VERSION_STRING_LEN);

    if (sscanf(otaVersionString, "v%u.%u.%u-", &version->major, &version->minor, &version->subMinor) != 3) {
        return false;
    }

    return true;
}

bool OtaIsVeryficationNeed(void)
{
    const esp_partition_t* runningPartition = esp_ota_get_running_partition();
    esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;

	esp_err_t err = esp_ota_get_state_partition(runningPartition, &state);
    ESP_LOGI(TAG, "ota veryfiction need %d", err);
    ESP_LOGI(TAG, "ota states %d", state);

	return (state == ESP_OTA_IMG_PENDING_VERIFY);
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static void CloseHttpClient(void)
{
    if(sFileClientHandler == NULL){
        return;
    }

    esp_http_client_close(sFileClientHandler);
	esp_http_client_cleanup(sFileClientHandler);
}

static void DelayTask(void)
{
    ESP_LOGI(TAG, "delay task");
    vTaskDelete(NULL);

    while (1){
        ;
    }
}

void OtaMainLoop(void *argument)
{
    Ota_t* otaCandidate = argument;

    ESP_LOGI(TAG, "start");
    ESP_LOGI(TAG, "url %s", otaCandidate->firmwareUrl);
    
    vTaskDelay(10U * 1000U);

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running){
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x", configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)", running->type, running->subtype, running->address);

    esp_http_client_config_t config = {
        .url = otaCandidate->firmwareUrl,
    };

    sOtaStatus = OTA_DOWNLOADING;
    
    ESP_LOGI(TAG, "client init");
    sFileClientHandler = esp_http_client_init(&config);
	if (sFileClientHandler == NULL){
        sOtaStatus = OTA_ERROR_INCORRECT_ADDRESS;
        ESP_LOGE(TAG, "can't init http client");
        DelayTask();
	}

    ESP_LOGI(TAG, "client open");
    esp_err_t err = esp_http_client_open(sFileClientHandler, 0);
	if (err != ESP_OK){
        sOtaStatus = OTA_ERROR_INCORRECT_ADDRESS;
	    ESP_LOGE(TAG, "can't open http client");
        esp_http_client_cleanup(sFileClientHandler);
        DelayTask();
	}
    
    ESP_LOGI(TAG, "fetch header");
    uint32_t totalFileSize = esp_http_client_fetch_headers(sFileClientHandler);
    if (totalFileSize == 0){
        sOtaStatus = OTA_ERROR_INCORRECT_SIZE;
        ESP_LOGE(TAG, "incorrect file size");
		CloseHttpClient();
        DelayTask();
	}

    ESP_LOGI(TAG, "file size %u", totalFileSize);

    esp_http_client_transport_t type = esp_http_client_get_transport_type(sFileClientHandler);
	int contentLen = esp_http_client_get_content_length(sFileClientHandler);

    ESP_LOGI(TAG, "type %d, len %d", type, contentLen);
    ESP_LOGI(TAG, "free partition");

    const esp_partition_t* updatePartition = esp_ota_get_next_update_partition(NULL);
    if (updatePartition == NULL){
        sOtaStatus = OTA_ERROR_PARTITION_PROBLEM;
		ESP_LOGE(TAG, "can't get free partition");
        CloseHttpClient();
        DelayTask();
	}

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x", updatePartition->subtype, updatePartition->address);
    ESP_LOGI(TAG, "ota begin");

    esp_ota_handle_t updatePartitionHandle = 0;

    err = esp_ota_begin(updatePartition, totalFileSize, &updatePartitionHandle);
	if (err == ESP_OK){
		ESP_LOGI(TAG, "Open update partition");
	}
    else{
        sOtaStatus = OTA_ERROR_PARTITION_PROBLEM;
        ESP_LOGE(TAG, "ota begin error %d", err);
        CloseHttpClient();
        DelayTask();
    }

	uint32_t currentReadDataFromBinFile = 0;
    uint32_t actualCrc = 0;

    char dataBuffer[DATA_BUFFOR_SIZE] = {};

    int64_t updtateStartTime = TimeDriverGetSystemTickMs();

    for(;;)
    {
        int singleDataPackageSize = esp_http_client_read(sFileClientHandler, dataBuffer, DATA_BUFFOR_SIZE);
        if(singleDataPackageSize > 0){
            currentReadDataFromBinFile += singleDataPackageSize;
            err = esp_ota_write(updatePartitionHandle, (uint8_t*)dataBuffer, singleDataPackageSize);
            actualCrc = esp_crc32_le(actualCrc, (uint8_t const *)dataBuffer, singleDataPackageSize);
            
            if(err == ESP_OK){
                ESP_LOGI(TAG, "downloaded %d, left to download %d", singleDataPackageSize, totalFileSize - currentReadDataFromBinFile);
            }else{
                sOtaStatus = OTA_ERROR_INCORRECT_DATA_IN_IMAGE;
                CloseHttpClient();
                esp_ota_abort(updatePartitionHandle);
                ESP_LOGE(TAG, "ota error %d", err);
                DelayTask();
            }
		}
        else if(singleDataPackageSize < 0){
            sOtaStatus = OTA_ERROR_READ_HTTP;
            CloseHttpClient();
            esp_ota_abort(updatePartitionHandle);
            ESP_LOGE(TAG, "read http client error %d", singleDataPackageSize);
            DelayTask();
        }
        else{
            if(esp_http_client_is_complete_data_received(sFileClientHandler) != true){
                sOtaStatus = OTA_ERROR_DOWNLOAD_TO_LONG_INCOMPLETE_FILE;
                ESP_LOGE(TAG, "error in receiving complete file");
                CloseHttpClient();
                esp_ota_abort(updatePartitionHandle);
                DelayTask();
            }

            CloseHttpClient();
            sOtaStatus = OTA_DOWNLOADED;

            ESP_LOGI(TAG, "end of download");
            ESP_LOGI(TAG, "crc actual calculate %X", actualCrc);
            ESP_LOGI(TAG, "crc %X", otaCandidate->checksum);

            if(actualCrc != otaCandidate->checksum){
                sOtaStatus = OTA_ERROR_PACKAGE_CHECK_VALUE_INCORRECT;
                ESP_LOGE(TAG, "incorrect crc");
                esp_ota_abort(updatePartitionHandle);
                DelayTask();
            }

            if((esp_ota_end(updatePartitionHandle) != ESP_OK)  || (esp_ota_set_boot_partition(updatePartition) != ESP_OK)){
                sOtaStatus = OTA_ERROR_INVALID_IMAGE;
                ESP_LOGE(TAG, "invalid image");
                esp_ota_abort(updatePartitionHandle);
                DelayTask();
            }
            ESP_LOGI(TAG, "save image in new partition");
            vTaskDelay(1000U);

            ESP_LOGI(TAG, "time for restart");
            McuDriverDeviceSafeRestart();
		}

        if(TimeDriverHasTimeElapsed(updtateStartTime, UPDATE_DOWNLOAD_TIMEOUT)){
            ESP_LOGE(TAG, "downloading takes too long");
            sOtaStatus = OTA_ERROR_DOWNLOAD_TO_LONG;
            CloseHttpClient();
            DelayTask();
        }

        vTaskDelay(TASK_DELAY_MS);
	}
}

esp_err_t OtaUploadByWebserver(httpd_req_t* req)
{
    char buf[DATA_BUFFOR_SIZE] = {};
    const uint32_t totalFileSize = req->content_len;

    ESP_LOGI(TAG, "start");
    ESP_LOGI(TAG, "total file size %u", totalFileSize);
    if (totalFileSize == 0){
        ESP_LOGE(TAG, "incorrect file size");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "free partition");
    const esp_partition_t* updatePartition = esp_ota_get_next_update_partition(NULL);
    if (updatePartition == NULL){
		ESP_LOGE(TAG, "can't get free partition");
        return ESP_FAIL;
	}

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x", updatePartition->subtype, updatePartition->address);
    ESP_LOGI(TAG, "ota begin");

    esp_ota_handle_t updatePartitionHandle = 0;
    esp_err_t err = esp_ota_begin(updatePartition, totalFileSize, &updatePartitionHandle);
	if (err == ESP_OK){
		ESP_LOGI(TAG, "Open update partition");
	}
    else{
        ESP_LOGE(TAG, "ota begin error %d", err);
        return ESP_FAIL;
    }

    uint32_t currentReadDataFromBinFile = 0;

    for(;;){
        /* Read the data for the request */
        int singleDataPackageSize = httpd_req_recv(req, buf, DATA_BUFFOR_SIZE);
        if (singleDataPackageSize > 0) {
            currentReadDataFromBinFile += singleDataPackageSize;
            err = esp_ota_write(updatePartitionHandle, (uint8_t*)buf, singleDataPackageSize);
            if(err == ESP_OK){
                ESP_LOGI(TAG, "upload %d, left to upload %d", singleDataPackageSize, totalFileSize - currentReadDataFromBinFile);
            }else{
                esp_ota_abort(updatePartitionHandle);
                ESP_LOGE(TAG, "ota error %d", err);
                
                return ESP_FAIL;
            }
        }
        else if(singleDataPackageSize < 0){
            esp_ota_abort(updatePartitionHandle);
            ESP_LOGE(TAG, "read http req error %d", singleDataPackageSize);

            return ESP_FAIL;
        }
        else{
            ESP_LOGI(TAG, "end of upload");

            if((esp_ota_end(updatePartitionHandle) != ESP_OK)  || (esp_ota_set_boot_partition(updatePartition) != ESP_OK)){
                ESP_LOGE(TAG, "invalid image");
                esp_ota_abort(updatePartitionHandle);

                return ESP_FAIL;
            }
            ESP_LOGI(TAG, "save image in new partition");
            vTaskDelay(1000U);
            
            return ESP_OK;
		}
    }

    

    return ESP_FAIL;
}