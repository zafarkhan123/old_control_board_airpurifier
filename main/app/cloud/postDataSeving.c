/**
 * @file postDataSeving.c
 *
 * @brief Post Data Seving middleware source file
 *
 * @dir postDataSeving
 * @brief postDataSeving middleware folder
 *
 * @author matfio
 * @date 2021.12.02
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "esp_log.h"

#include "cloud/postDataSeving.h"

#include "utils/circQueue/circQueue.h"

#include "nvsDriver/nvsDriver.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define SINGLE_POST_STATUS_ELEMENT_SIZE (sizeof(messageTypeDeviceStatusHttpClient_t))
#define POST_STATUS_BUFFER_SIZE ((SINGLE_POST_STATUS_ELEMENT_SIZE) * (CFG_HTTP_CLIENT_NO_INTERNET_ACCESS_NUMBER_OF_SAVED_POST))

#define NVS_KAY_NAME ("postData")

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS
*****************************************************************************/

typedef struct
{
    uint16_t postNumber;
    messageTypeDeviceStatusHttpClient_t postStatus[CFG_HTTP_CLIENT_NO_INTERNET_ACCESS_NUMBER_OF_SAVED_POST];
} __attribute__ ((packed)) PostStatusData_t;

/*****************************************************************************
                     PRIVATE VARIABLES
*****************************************************************************/

static const char* TAG = "postDataSeving";

static uint8_t sPostStatusBuffer[POST_STATUS_BUFFER_SIZE];

static circQueue_t sCircQueue;

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

/** @brief Load post data fron nvs
 *  @param circQueue_t [in] pointer to circQueue_t structure
 *  @return return true if success
 */
static bool LoadPostStatusData(circQueue_t* circQueue);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool PostDataSevingInit(void)
{
    bool res = true;

    res = CircQueueStaticBufferInit(&sCircQueue, sPostStatusBuffer, POST_STATUS_BUFFER_SIZE, SINGLE_POST_STATUS_ELEMENT_SIZE);
    
    ESP_LOGI(TAG, "init result %d", res);
    ESP_LOGI(TAG, "single element size %d", SINGLE_POST_STATUS_ELEMENT_SIZE);
    ESP_LOGI(TAG, "buffer size %d", POST_STATUS_BUFFER_SIZE);

    LoadPostStatusData(&sCircQueue);

    return res;
}


void PostDataSevingWriteNvs(messageTypeDeviceStatusHttpClient_t* statusPost)
{
    PostStatusData_t postStatusData = {};

    ESP_LOGI(TAG, "add new element");
    CircQueueWrite(&sCircQueue, statusPost);

    uint16_t unreadElements = CircQueueReadSize(&sCircQueue);

    postStatusData.postNumber = unreadElements;
    ESP_LOGI(TAG, "now element %d", unreadElements);

    CircQueuePeek(&sCircQueue, postStatusData.postStatus, unreadElements);
    
    NvsDriverSave(NVS_KAY_NAME, &postStatusData, sizeof(PostStatusData_t));
}

uint16_t PostDataSevingReadSize(void)
{
    return CircQueueReadSize(&sCircQueue);
}

bool  PostDataSevingRead(messageTypeDeviceStatusHttpClient_t* statusPost)
{
    ESP_LOGI(TAG, "read element");
    return (CircQueueRead(&sCircQueue, statusPost, 1) == 1);
}

bool PostDataSevingClear(void)
{
    ESP_LOGI(TAG, "clear");

    PostStatusData_t postStatusData = {};

    return NvsDriverSave(NVS_KAY_NAME, &postStatusData, sizeof(PostStatusData_t));
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool LoadPostStatusData(circQueue_t* circQueue)
{
    PostStatusData_t postStatusData = {};
    uint16_t postStatusDataLen = sizeof(PostStatusData_t);

    bool nvsRes = NvsDriverLoad(NVS_KAY_NAME, &postStatusData, &postStatusDataLen);
    ESP_LOGI(TAG, "load data len %d", postStatusDataLen);

    if(nvsRes == true){
        ESP_LOGI(TAG, "load post status from nvs");
        ESP_LOGI(TAG, "Read %d element from nvs", postStatusData.postNumber);
        
        if(postStatusData.postNumber == 0){
            ESP_LOGI(TAG, "there is nothing to restore");

            return nvsRes;
        }

        if(postStatusData.postNumber > CFG_HTTP_CLIENT_NO_INTERNET_ACCESS_NUMBER_OF_SAVED_POST){
            ESP_LOGE(TAG, "to many elements to read");

            return false;
        }

        for(uint16_t idx = 0; idx < postStatusData.postNumber; ++idx){
            CircQueueWrite(circQueue, &postStatusData.postStatus[idx]);

            ESP_LOGI(TAG, "load element %d", idx + 1);
        }
    }
    else{
        ESP_LOGI(TAG, "save default post status nvs");

        NvsDriverSave(NVS_KAY_NAME, &postStatusData, sizeof(PostStatusData_t));
    }

    return nvsRes;
}