/**
 * @file iotHubClient.c
 *
 * @brief iotHubClient source file
 *
 * @dir cloud
 * @brief cloud folder
 *
 * @author matfio
 * @date 2022.03.04
 * @version v1.0
 *
 * @copyright 2022 Fideltronik R&D - all rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "setting.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "iotHubClient.h"

#include "iothub_client.h"
#include "iothub.h"
#include "iothub_message.h"
#include "iothub_client_version.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/http_proxy_io.h"

#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"
#include "sdkconfig.h"

#include "iothubtransportmqtt.h"
#include "azure_prov_client/prov_transport_mqtt_client.h"

#include "iothubtransportmqtt_websockets.h"
#include "azure_prov_client/prov_transport_mqtt_ws_client.h"

#include "esp_log.h"

#include "scheduler/scheduler.h"
#include "factorySettingsDriver/factorySettingsDriver.h"
#include "timeDriver/timeDriver.h"
#include "timerDriver/timerDriver.h"
#include "rtcDriver/rtcDriver.h"
#include "nvsDriver/nvsDriver.h"

#include "postDataSeving.h"

#include "location/location.h"
#include "utils/multiBuffer/multiBuffer.h"

#include "common/messageType.h"
#include "common/messageParserAndSerializer.h"

#include "device/alarmHandling.h"
#include "webServer/webServer.h"

#include "iothubtransporthttp.h"
#include "azure_prov_client/prov_transport_http_client.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define PROV_SLEEP_TIMIE_MS                                 (100U)
#define PROV_TIMEOUT_SEC                                    (1U * 60U)

#define IOTHUB_SLEEP_TIMIE_MS                               (100U)
#define IOTHUB_KEEP_ALIVE_SEC                               (30U)

#define MUTEX_TIMEOUT_MS                                    (5U  * 1000U)
#define SOCKET_CONNECTION_TASK_DELAY_MS                     (1U * 1000U)

#define SEND_DEVICE_STATUS_UPDATE_REQUEST_INTERVAL_MS       (20U * 60U * 1000U)             // 20 minute
#define SEND_DEVICE_LOCATION_UPDATE_REQUEST_INTERVAL_MS     (12U * 60U * 60U * 1000U)       // 12 hours
#define SEND_DEVICE_SCHEDULE_UPDATE_REQUEST_INTERVAL_MS     (12U * 60U * 60U * 1000U)       // 12 hours

#define NVS_KAY_NAME ("iotHubSetting")

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

typedef enum
{
    DIRECT_MESSAGE_RETURN_STATUS_OK         = 200,
    DIRECT_MESSAGE_RETURN_STATUS_ERROR      = 500,
}DirectMessageReturnStatus_t;

typedef enum
{
    IOT_HUB_CONNECTION_STATUS_IDLE          = 0,
    IOT_HUB_CONNECTION_STATUS_CONNECTED        ,
    IOT_HUB_CONNECTION_STATUS_ERROR            ,
}IotHubConnectionStatus_t;

typedef enum
{
    PROV_CONNECTION_STATUS_IDLE             = 0,
    PROV_HUB_CONNECTION_STATUS_CONNECTED       ,
    PROV_HUB_CONNECTION_STATUS_ERROR           ,
}ProvConnectionStatus_t;

typedef struct
{
    char* iothub_uri;
    char* device_id;
    ProvConnectionStatus_t registration_complete;
}__attribute__ ((packed)) PROV_SAMPLE_INFO;

typedef struct
{
    IotHubConnectionStatus_t connected;
}__attribute__ ((packed)) IOTHUB_CLIENT_SAMPLE_INFO;

static const char* TAG = "iotHubClient";

static bool sTraceOn = false;

static IOTHUB_DEVICE_CLIENT_LL_HANDLE sIoTHubDeviceHandle;
static IOTHUB_CLIENT_SAMPLE_INFO sIoTHubInfo;
static PROV_SAMPLE_INFO sProvInfo;

static SemaphoreHandle_t sSettingMutex;
static iotHubClientStatus_t sIotHubClientStatus;

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief The callback that gets called on registration or if an error is encountered
 *  @param register_result registration result
 *  @param iothub_uri prov uri string
 *  @param device_id device id string
 *  @param user_context user context
 */
static void ProvRegisterDeviceCallback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context);

/** @brief Registration status callback used to inform the caller of registration status
 *  @param register_result registration result
 *  @param user_context user context
 */
static void ProvRegistrationStatusCallback(PROV_DEVICE_REG_STATUS reg_status, void* user_context);

/** @brief The callback specified by the device for receiving updates about the status of the connection to IoT Hub
 *  @param result registration status result
 *  @param reason connection status reason
 *  @param user_context user context
 */
static void IotHubConnectionStatus(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context);

/** @brief Is device status change and send POST request is needed
 *  @param settingFirst [in] pointer to SettingDevice_t struct
 *  @param settingSecond [in] pointer to SettingDevice_t struct
 *  @return true if send POST needed
 */
static bool IsSendPostNeedBecauseDeviceStatusChange(const SettingDevice_t* settingFirst, const SettingDevice_t* settingSecond);

/** @brief Add post data to nvs when device is not connect to internet
 *  @param setting [in] pointer to SettingDevice_t struct
 *  @return true if success
 */
static bool SavePostDeviceStatus(const SettingDevice_t* setting);

/** @brief Send device info
 *  @return true if success
 */
static bool SendDeviceInfo(void);

/** @brief Send device status
 *  @param setting [in] pointer to SettingDevice_t struct
 *  @return true if success
 */
static bool SendDeviceStatus(const SettingDevice_t* setting);

/** @brief Send device location
 *  @param location [in] pointer to Location_t struct
 *  @return true if success
 */
static bool SendDeviceLocation(const Location_t* location);

/** @brief Send device scheduler
 *  @param scheduler [in] pointer to Scheduler_t struct
 *  @return true if success
 */
static bool SendDeviceScheduler(const Scheduler_t* scheduler);

/** @brief Publish send data to iot hub by direct method
 *  @param scheduler [in] pointer to Scheduler_t struct
 *  @return true if success
 */
static bool PublishDataEvent(char* text, int textLen);

/** @brief Read new device location
 *  @param jsonBody [in] json string 
 *  @param location [out] pointer to Location_t struct
 *  @return true if success
 */
static bool ReadDeviceLocation(const char * const jsonBody, Location_t* location);

/** @brief Read new device scheduler
 *  @param jsonBody [in] json string 
 *  @param scheduler [out] pointer to Scheduler_t struct
 *  @return true if success
 */
static bool ReadDeviceScheduler(const char * const jsonBody, Scheduler_t* scheduler);

/** @brief Read new device mode
 *  @param jsonBody [in] json string 
 *  @param setting [out] pointer to SettingDevice_t struct
 *  @return true if success
 */
static bool ReadDeviceMode(const char * const jsonBody, SettingDevice_t* setting);

/** @brief Read new device service
 *  @param jsonBody [in] json string 
 *  @return true if success
 */
static bool ReadDeviceService(const char * const jsonBody);

/** @brief Read new device update
 *  @param jsonBody [in] json string 
 *  @return true if success
 */
static bool ReadDeviceUpdate(const char * const jsonBody);

/** @brief The callback which will be called by IoTHub when recaive data from direct method
 *  @param method_name nethod name string
 *  @param payload recaive string payload
 *  @param size recaive string size
 *  @param response response string
 *  @param response_size response string size
 *  @param userContextCallback user context callback
 *  @return 0 if success
 */
static int DeviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback);

/** @brief Provisioning initialization
 *  @return true if success
 */
static bool ProvisioningInit(void);

/** @brief IoT Hub Client initialization
 */
static void IoTHubClientInit(void);

/** @brief IoT Hub Client deinit
 */
static void IoTHubClientDeInit(void);

/** @brief Send saved data status
 *  @return true if success
 */
static bool SendSavedDeviceStatus(void);

/** @brief Initialize IoT Hub Client setting struct
 *  @return true if success
 */
static bool IotHubClientSettingLoad(void);

/** @brief Execute recaive payload from direct message
 *  @param message recaive message
 *  @return true if success
 */
static bool ExecuteReadDirectMessage(const unsigned char* message);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

const IO_INTERFACE_DESCRIPTION* socketio_get_interface_description(void)
{
    return NULL;
}

bool IotHubClientInit(void)
{
    sSettingMutex = xSemaphoreCreateMutex();
    if (sSettingMutex == NULL) {
        return false;
    }

    IotHubClientSettingLoad();

    return true;
}

bool IotHubClientGetSetting(iotHubClientStatus_t* setting)
{
    if (xSemaphoreTake(sSettingMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        memcpy(setting, &sIotHubClientStatus, sizeof(iotHubClientStatus_t));

        xSemaphoreGive(sSettingMutex);
        return true;
    }
    
    return false;
}

bool IotHubClientSetSetting(iotHubClientStatus_t* setting)
{
    if (xSemaphoreTake(sSettingMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        memcpy(&sIotHubClientStatus, setting, sizeof(iotHubClientStatus_t));

        xSemaphoreGive(sSettingMutex);
        return true;
    }
    
    return false;
}

bool IotHubClientSettingSave(void)
{
    if (xSemaphoreTake(sSettingMutex, MUTEX_TIMEOUT_MS) == pdTRUE) {
        bool res = NvsDriverSave(NVS_KAY_NAME, &sIotHubClientStatus, sizeof(iotHubClientStatus_t));

        xSemaphoreGive(sSettingMutex);
        return res;
    }

    return false;
}

void IotHubClientMainLoop(void *argument)
{
    ESP_LOGI(TAG, "IoT Hub main loop start");
    
    bool runFirstTime = true;
    bool runFirstTimeWithoutInternet = true;
    bool initResult = true;

    bool isProvSuccess = false;
    bool isIoTHubInit = false;

    int64_t sendMessageDeviceStatusTime = 0;
    int64_t sendMessageDeviceLocationTime = 0;
    int64_t sendMessageDeviceSchedulerTime = 0;

    SettingDevice_t setting = {};
    SettingDevice_t settingOld = {};
    Location_t location = {};
    Location_t locationOld = {};
    Scheduler_t scheduler = {};
    Scheduler_t schedulerOld = {};

    vTaskDelay(5U * 1000U);

    initResult &= SettingGet(&settingOld);
    initResult &= LocationGet(&locationOld);
    initResult &= SchedulerGetAll(&schedulerOld);
    initResult &= PostDataSevingInit();
    assert(initResult);

    for (;;)
    {
        SettingGet(&setting);
        LocationGet(&location);
        SchedulerGetAll(&scheduler);

        bool isConnectedToInternet = false;

        if((setting.ethernetPcbAdded == true) && (setting.ethernetStatus == ETHERNET_EVENT_CONNECTED)){
            isConnectedToInternet = true;
        }

        if((setting.restore.isWifiOn == true) && (setting.wifiStatus == WIFI_STATUS_STA_CONNECTED)){
            isConnectedToInternet = true;
        }

        if((sIoTHubInfo.connected == IOT_HUB_CONNECTION_STATUS_ERROR) && (isConnectedToInternet == true)){
            // the connection to the cloud has been broken
            isConnectedToInternet = false;
            sIoTHubInfo.connected = IOT_HUB_CONNECTION_STATUS_IDLE;
        }

        if(isConnectedToInternet == false){            
            // Save post status to nvs
            bool deviceStatusChange = IsSendPostNeedBecauseDeviceStatusChange(&setting, &settingOld);
            if((sIotHubClientStatus.isConnectedLeastOnce == true) && ((runFirstTimeWithoutInternet == true) || (deviceStatusChange == true))){
                ESP_LOGI(TAG, "seve device status to nvs");

                SettingGet(&setting);
                SavePostDeviceStatus(&setting);

                memcpy(&settingOld, &setting, sizeof(SettingDevice_t));

                runFirstTimeWithoutInternet = false;
            }

            if(sIoTHubDeviceHandle != NULL){
                IoTHubClientDeInit();

                isProvSuccess = false;
                isIoTHubInit = false;
            }

            vTaskDelay(SOCKET_CONNECTION_TASK_DELAY_MS);
            continue;
        }

        if(isProvSuccess == false){
            vTaskDelay(5U * 1000U);

            isProvSuccess = ProvisioningInit();
            if(isProvSuccess == false){
                vTaskDelay(SOCKET_CONNECTION_TASK_DELAY_MS);
                continue;
            }
        }

        if(isIoTHubInit == false){
            IoTHubClientInit();
            isIoTHubInit = true;
        }
        
        if (sIoTHubInfo.connected == IOT_HUB_CONNECTION_STATUS_CONNECTED){

            if((sIotHubClientStatus.isConnectedLeastOnce == false) && (setting.wifiStatus == WIFI_STATUS_STA_CONNECTED)){
                ESP_LOGI(TAG, "IotHub connected least once");
                sIotHubClientStatus.isConnectedLeastOnce =  true;
                IotHubClientSettingSave();
                continue;
            }

            uint16_t unsendPostStatusRequestNumber = PostDataSevingReadSize();
            if(unsendPostStatusRequestNumber != 0){
                ESP_LOGI(TAG, "element to resend %d", unsendPostStatusRequestNumber);
                if(SendSavedDeviceStatus() == true){
                    ESP_LOGI(TAG, "all old post send");
                }
                else{
                    ESP_LOGW(TAG, "cannot send all old post");
                }
            }

            if(runFirstTime == true){
                sIotHubClientStatus.lastConnection = TimeDriverGetUTCUnixTime();
                IotHubClientSettingSave();

                ESP_LOGI(TAG, "last connection time %u", sIotHubClientStatus.lastConnection);

                if(SendDeviceInfo() == true){
                    ESP_LOGI(TAG, "send device info");
                }
                else{
                    ESP_LOGW(TAG, "cannot send device info");
                }
            }

            bool deviceStatusChange = IsSendPostNeedBecauseDeviceStatusChange(&setting, &settingOld);
            if((runFirstTime == true) || (deviceStatusChange == true) || (TimeDriverHasTimeElapsed(sendMessageDeviceStatusTime, SEND_DEVICE_STATUS_UPDATE_REQUEST_INTERVAL_MS))){
                SettingGet(&setting);

                if(SendDeviceStatus(&setting) == true){
                    sendMessageDeviceStatusTime = TimeDriverGetSystemTickMs();
                    memcpy(&settingOld, &setting, sizeof(SettingDevice_t));
                    ESP_LOGI(TAG, "send device status");
                }
                else{
                    ESP_LOGW(TAG, "cannot send device status");
                }
            }

             bool locationChange = (memcmp(&location, &locationOld, sizeof(Location_t)) != 0);
            if((runFirstTime == true) || (locationChange == true) || (TimeDriverHasTimeElapsed(sendMessageDeviceLocationTime, SEND_DEVICE_LOCATION_UPDATE_REQUEST_INTERVAL_MS))){
                LocationGet(&location);

                if(SendDeviceLocation(&location) == true){
                    sendMessageDeviceLocationTime = TimeDriverGetSystemTickMs();
                    memcpy(&locationOld, &location, sizeof(Location_t));
                    ESP_LOGI(TAG, "send device location");
                }
                else{
                    ESP_LOGW(TAG, "cannot send device location");
                }
            }

            bool schedulerChange = (memcmp(&scheduler, &schedulerOld, sizeof(Scheduler_t)) != 0);
            if((runFirstTime == true) || (schedulerChange == true) || (TimeDriverHasTimeElapsed(sendMessageDeviceSchedulerTime, SEND_DEVICE_SCHEDULE_UPDATE_REQUEST_INTERVAL_MS))){
                SchedulerGetAll(&scheduler);

                if(SendDeviceScheduler(&scheduler) == true){
                    sendMessageDeviceSchedulerTime = TimeDriverGetSystemTickMs();
                    memcpy(&schedulerOld, &scheduler, sizeof(Scheduler_t));
                    ESP_LOGI(TAG, "send device scheduler");
                }
                else{
                    ESP_LOGW(TAG, "cannot send device scheduler");
                }
            }

            runFirstTime = false;
        }
        IoTHubDeviceClient_LL_DoWork(sIoTHubDeviceHandle);
        ThreadAPI_Sleep(IOTHUB_SLEEP_TIMIE_MS);
    }
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool PublishDataEvent(char* text, int textLen)
{
    IOTHUB_MESSAGE_HANDLE msg_handle = IoTHubMessage_CreateFromByteArray((const unsigned char*)text, textLen);
    if (msg_handle != NULL){
        if (IoTHubDeviceClient_LL_SendEventAsync(sIoTHubDeviceHandle, msg_handle, NULL, NULL) != IOTHUB_CLIENT_OK){
            ESP_LOGE(TAG, "IoTHubClient_LL_SendEventAsync..........FAILED!");
        }

        IoTHubMessage_Destroy(msg_handle);

        return true;
    }

    return false;
}

static void ProvRegisterDeviceCallback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    if (user_context == NULL){
        ESP_LOGE(TAG, "user_context is NULL");

        return;
    }

    // cppcheck-suppress shadowVariable
    PROV_SAMPLE_INFO* user_ctx = (PROV_SAMPLE_INFO*)user_context;
    
    if (register_result == PROV_DEVICE_RESULT_OK){
        ESP_LOGI(TAG, "Registration Information received from service: %s deviceId %s", iothub_uri, device_id);
        mallocAndStrcpy_s(&user_ctx->iothub_uri, iothub_uri);
        mallocAndStrcpy_s(&user_ctx->device_id, device_id);
        user_ctx->registration_complete = PROV_HUB_CONNECTION_STATUS_CONNECTED;
    }
    else{
        ESP_LOGE(TAG, "Failure encountered on registration %d", register_result);
        user_ctx->registration_complete = PROV_HUB_CONNECTION_STATUS_ERROR;
    }
}

static void ProvRegistrationStatusCallback(PROV_DEVICE_REG_STATUS reg_status, void* user_context)
{
    ESP_LOGI(TAG, "Provisioning Status: %d", reg_status);
}

static void IotHubConnectionStatus(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    if (user_context == NULL){
        ESP_LOGE(TAG, "iothub_connection_status user_context is NULL");

        return;
    }

    ESP_LOGI(TAG, "Connection result: %d reason: %d", result, reason);

    // cppcheck-suppress shadowVariable
    IOTHUB_CLIENT_SAMPLE_INFO* sIoTHubInfo = (IOTHUB_CLIENT_SAMPLE_INFO*)user_context;
    if ((result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) && (reason == IOTHUB_CLIENT_CONNECTION_OK)){
        sIoTHubInfo->connected = IOT_HUB_CONNECTION_STATUS_CONNECTED;
    }
    else{
        sIoTHubInfo->connected = IOT_HUB_CONNECTION_STATUS_ERROR;
    }    
}

static bool IsSendPostNeedBecauseDeviceStatusChange(const SettingDevice_t* settingFirst, const SettingDevice_t* settingSecond)
{
    if(settingFirst->restore.deviceStatus.isDeviceOn != settingSecond->restore.deviceStatus.isDeviceOn){
        return true;
    }

    if(settingFirst->restore.deviceStatus.fanLevel != settingSecond->restore.deviceStatus.fanLevel){
        return true;
    }

    if(settingFirst->restore.deviceStatus.isEkoOn != settingSecond->restore.deviceStatus.isEkoOn){
        return true;
    }

    if(settingFirst->restore.touchLock != settingSecond->restore.touchLock){
        return true;
    }

    if(settingFirst->restore.deviceMode != settingSecond->restore.deviceMode){
        return true;
    }

    if(memcmp(&settingFirst->alarmWarning, &settingSecond->alarmWarning, sizeof(SettingAlarmWarning_t)) != 0){
        return true;
    }

    if(memcmp(&settingFirst->alarmError, &settingSecond->alarmError, sizeof(SettingAlarmError_t)) != 0){
        return true;
    }

    if(memcmp(&settingFirst->timersStatus, &settingSecond->timersStatus, sizeof(SettingTimersStatus_t)) != 0){
        return true;
    }

    return false;
}

static bool SavePostDeviceStatus(const SettingDevice_t* setting)
{
    messageTypeDeviceStatusHttpClient_t deviceStatus = {};

    MessageTypeCreateDeviceStatusHttpClient(&deviceStatus, setting);

    PostDataSevingWriteNvs(&deviceStatus);

    return true;
}

static bool SendDeviceInfo(void)
{
    char jsonStr[MESSAGE_TYPE_MAX_DEVICE_INFO_JSON_LENGTH] = {};
    messageTypeDeviceInfoHttpClient_t deviceInfo = {};
    
    MessageTypeCreateDeviceInfoHttpClient(&deviceInfo);
    
    cJSON *jsonRoot = cJSON_CreateObject();

    MessageParserAndSerializerCreateDeviceInfoHttpClientJson(jsonRoot, &deviceInfo);

    if (cJSON_PrintPreallocated(jsonRoot, jsonStr, MESSAGE_TYPE_MAX_DEVICE_INFO_JSON_LENGTH, false) == false) {
        jsonStr[0] = 0;
        ESP_LOGE(TAG, "device info json size is too big");
        
        return false;
    }

    cJSON_Delete(jsonRoot);

    int jsonStrLen = strlen(jsonStr);
    ESP_LOGI(TAG, "device info len %d", jsonStrLen);
    ESP_LOGI(TAG, "%s", jsonStr);

    bool sendStatus = PublishDataEvent(jsonStr, jsonStrLen);
    if(sendStatus == false){
        ESP_LOGW(TAG, "device info websocket send error");
        
        return false;
    }

    return true;
}

static bool SendDeviceStatus(const SettingDevice_t* setting)
{
    char jsonStr[MESSAGE_TYPE_MAX_DEVICE_STATUS_JSON_LENGTH] = {};
    messageTypeDeviceStatusHttpClient_t deviceStatus = {};

    MessageTypeCreateDeviceStatusHttpClient(&deviceStatus, setting);

    cJSON *jsonRoot = cJSON_CreateObject();

    MessageParserAndSerializerCreateDeviceStatusHttpClientJson(jsonRoot, &deviceStatus);

    if (cJSON_PrintPreallocated(jsonRoot, jsonStr, MESSAGE_TYPE_MAX_DEVICE_STATUS_JSON_LENGTH, false) == false) {
        jsonStr[0] = 0;
        ESP_LOGE(TAG, "device status json size is too big");
        
        return false;
    }

    cJSON_Delete(jsonRoot);

    int jsonStrLen = strlen(jsonStr);
    ESP_LOGI(TAG, "device status len %d", jsonStrLen);
    ESP_LOGI(TAG, "%s", jsonStr);

    bool sendStatus = PublishDataEvent(jsonStr, jsonStrLen);
    if(sendStatus == false){
        ESP_LOGW(TAG, "device status websocket send error");
        
        return false;
    }

    return true;
}

static bool SendDeviceLocation(const Location_t* location)
{
    char jsonStr[MESSAGE_TYPE_MAX_DEVICE_LOCATION_JSON_LENGTH] = {};
    cJSON *jsonRoot = cJSON_CreateObject();

    MessageParserAndSerializerCreateDeviceLocationHttpClientJson(jsonRoot, location);

    if (cJSON_PrintPreallocated(jsonRoot, jsonStr, MESSAGE_TYPE_MAX_DEVICE_LOCATION_JSON_LENGTH, false) == false) {
        jsonStr[0] = 0;
        ESP_LOGE(TAG, "device location json size is too big");
        
        return false;
    }

    cJSON_Delete(jsonRoot);

    int jsonStrLen = strlen(jsonStr);
    ESP_LOGI(TAG, "device location len %d", jsonStrLen);
    ESP_LOGI(TAG, "%s", jsonStr);

    bool sendStatus = PublishDataEvent(jsonStr, jsonStrLen);
    if(sendStatus == false){
        ESP_LOGW(TAG, "device location websocket send error");
        
        return false;
    }

    return true;
}

static bool SendDeviceScheduler(const Scheduler_t* scheduler)
{
    messageTypeScheduler_t messageScheduler = {};
    char jsonStr[MESSAGE_TYPE_MAX_DEVICE_SCHEDULER_JSON_LENGTH] = {};

    cJSON *jsonRoot = cJSON_CreateObject();

    MessageTypeCreateMessageTypeScheduler(&messageScheduler, scheduler);
    MessageParserAndSerializerCreateSchedulerJson(jsonRoot, &messageScheduler);

    if (cJSON_PrintPreallocated(jsonRoot, jsonStr, MESSAGE_TYPE_MAX_DEVICE_SCHEDULER_JSON_LENGTH, false) == false) {
        jsonStr[0] = 0;
        ESP_LOGE(TAG, "device scheduler json size is too big");
      
        return false;
    }

    cJSON_Delete(jsonRoot);

    int jsonStrLen = strlen(jsonStr);
    ESP_LOGI(TAG, "device scheduler len %d", jsonStrLen);
    ESP_LOGI(TAG, "%s", jsonStr);

    bool sendStatus = PublishDataEvent(jsonStr, jsonStrLen);
    if(sendStatus == false){
        ESP_LOGW(TAG, "device scheduler websocket send error");
        
        return false;
    }

    return true;
}

static bool ReadDeviceLocation(const char * const jsonBody, Location_t* location)
{
    Location_t tempLocation = {};
    memcpy(&tempLocation, location, sizeof(Location_t));

    bool parseStat = MessageParserAndSerializerParseDeviceLocationHttpClientJsonString(jsonBody, &tempLocation);
    int cmpRes = memcmp(location, &tempLocation, sizeof(Location_t));

    if(cmpRes == 0){
        ESP_LOGI(TAG, "set the same location");
        return true;
    }

    if(parseStat == true){
        ESP_LOGI(TAG, "location changed");
        LocationSet(&tempLocation);
        LocationSave();
        LocationPrintf(&tempLocation);

        return true;
    }
    
    return false;
}

static bool ReadDeviceScheduler(const char * const jsonBody, Scheduler_t* scheduler)
{
    Scheduler_t tempScheduler = {};
    memcpy(&tempScheduler, scheduler, sizeof(Scheduler_t));

    messageTypeScheduler_t tempMessageTypeScheduler = {};
    bool parseRes = MessageParserAndSerializerParseDeviceSchedulerJsonString(jsonBody, &tempMessageTypeScheduler);
    MessageTypeCreateScheduler(&tempMessageTypeScheduler, &tempScheduler);

    int cmpRes = memcmp(scheduler, &tempScheduler, sizeof(Scheduler_t));

    if(cmpRes == 0){
        ESP_LOGI(TAG, "set the same scheduler");
        return true;
    }

    if(parseRes == true){
        ESP_LOGI(TAG, "scheduler changed");

        SchedulerSetAll(&tempScheduler);
        SchedulerSave();
        SchedulerPrintf(&tempScheduler); 

        return true;
    }

    return false;
}

static bool ReadDeviceMode(const char * const jsonBody, SettingDevice_t* setting)
{                  
    SettingDevice_t tempSetting = {};
    memcpy(&tempSetting, setting, sizeof(SettingDevice_t));

    messageTypeDeviceModeHttpClient_t tempDeviceMode = {};
    MessageTypeCreateDeviceModeHttpClientFromSetting(&tempDeviceMode, &tempSetting);

    bool parseRes = MessageParserAndSerializerParseDeviceModeHttpClientJsonString(jsonBody, &tempDeviceMode);
    MessageTypeCreateSettingFromDeviceModeHttpClient(&tempDeviceMode, &tempSetting);
    if(parseRes == true){
        int cmpRes = memcmp(&setting->restore.deviceStatus, &tempSetting.restore.deviceStatus, sizeof(SettingDeviceStatust_t));
        if(cmpRes != 0){
            ESP_LOGI(TAG, "device status change");
            SettingUpdateDeviceStatus(&tempSetting);
        }

        bool isModeChange = (setting->restore.deviceMode != tempSetting.restore.deviceMode);
        if(isModeChange){
            ESP_LOGI(TAG, "device mode change");
            SettingUpdateDeviceMode(&tempSetting);
        }
        
        bool isLockChange = (setting->restore.touchLock != tempSetting.restore.touchLock);
        if(isLockChange){
            ESP_LOGI(TAG, "touch lock change");
            SettingUpdateTouchScreen(tempSetting.restore.touchLock);
        }

        return true;
    }
    return false;
}

static bool ReadDeviceService(const char * const jsonBody)
{
    messageTypeDeviceServiceHttpClient_t deviceService = {};
    bool parseRes = MessageParserAndSerializerParseDeviceServiceHttpClientJsonString(jsonBody, &deviceService);
    if(parseRes == true){
        if(deviceService.deviceReset == true){
            ESP_LOGI(TAG, "restart device");
            SettingDevice_t setting = {};

            SettingGet(&setting);
            setting.deviceReset = true;
            SettingSet(&setting);
        }

        if(deviceService.uv1TimerReload == true){
            ESP_LOGI(TAG, "uv1TimerReload");
            SettingDevice_t setting = {};
            SettingGet(&setting);

            TimerDriverClearCounter(TIMER_NAME_UV_LAMP_1);

            TimerDriverUpdateTimerSetting(&setting);
            AlarmHandlingTimersWornOutCheck(&setting);

            SettingSet(&setting);
            SettingSave();
        }

        if(deviceService.uv2TimerReload == true){
            ESP_LOGI(TAG, "uv2TimerReload");
            SettingDevice_t setting = {};
            SettingGet(&setting);

            TimerDriverClearCounter(TIMER_NAME_UV_LAMP_2);

            TimerDriverUpdateTimerSetting(&setting);
            AlarmHandlingTimersWornOutCheck(&setting);

            SettingSet(&setting);
            SettingSave();
        }

        if(deviceService.hepaTimerReload == true){
            ESP_LOGI(TAG, "hepaTimerReload");
            SettingDevice_t setting = {};
            SettingGet(&setting);

            TimerDriverClearCounter(TIMER_NAME_HEPA);

            TimerDriverUpdateTimerSetting(&setting);
            AlarmHandlingTimersWornOutCheck(&setting);

            SettingSet(&setting);
            SettingSave();
        }

        if(deviceService.scheduleReset == true){
            ESP_LOGI(TAG, "scheduleReset");

            bool res = true;
            Scheduler_t factoryScheduler = {};
            
            res &= FactorySettingsGetScheduler(&factoryScheduler);
            if(res == false){
                ESP_LOGE(TAG, "read factory setting fail");

                return false;
            }
         
            res &= SchedulerSetAll(&factoryScheduler);
                
            ESP_LOGI(TAG, "restore factory scheduler %d", res);
        }
        
        if(deviceService.utcTimeOffsetIsSet == true){
            float offset = LocationGetUtcOffset();

            if(offset != deviceService.utcTimeOffset){
                offset = deviceService.utcTimeOffset;
                ESP_LOGI(TAG, "utc offset change %.1f", offset);

                LocationSetUtcOffset(offset);
                LocationSave();

            }
        }

        if(deviceService.rtcTimeIsSet == true){
            ESP_LOGI(TAG, "actual esp time %u", TimeDriverGetUTCUnixTime());
            ESP_LOGI(TAG, "new esp time %u", deviceService.rtcTime);    

            struct tm * timeInfo;
            time_t rawTime = deviceService.rtcTime;

            timeInfo = localtime(&rawTime);

            RtcDriverSetDateTime(timeInfo);
            TimeDriverSetEspTime(timeInfo);

            SettingSave();
        }
        
        if(deviceService.hepaLivespanIsSet == true){
            bool res = true;
            uint32_t livespan = deviceService.hepaLivespan;

            res = FactorySettingsUpdateServiceParam(FACTORY_SETTING_SERVICE_HEPA_LIFETIME_HOURS, &livespan);
            if(res == false){
                ESP_LOGE(TAG, "new hepa livespan fault");

                return false;
            }

            ESP_LOGI(TAG, "new hepa livespan %u", livespan);
        }

        if(deviceService.hepaWarningIsSet == true){
            bool res = true;
            uint32_t livespan = deviceService.hepaWarning;

            res &= FactorySettingsUpdateServiceParam(FACTORY_SETTING_SERVICE_HEPA_WARNING_HOURS, &livespan);
            if(res == false){
                ESP_LOGE(TAG, "new hepa warning fault");
                 return false;
            }
            
            ESP_LOGI(TAG, "new hepa warning %u", livespan);
        }

        if(deviceService.uvLivespanIsSet == true){
            bool res = true;
            uint32_t livespan = deviceService.uvLivespan;

            res &= FactorySettingsUpdateServiceParam(FACTORY_SETTING_SERVICE_UV_LIFETIME_HOURS, &livespan);
            if(res == false){
                ESP_LOGE(TAG, "new hepa livespan fault");
                 return false;
            }

            ESP_LOGI(TAG, "new hepa livespan %u", livespan);
        }

        if(deviceService.uvWarningIsSet == true){
            bool res = true;
            uint32_t livespan = deviceService.uvWarning;

            res &= FactorySettingsUpdateServiceParam(FACTORY_SETTING_SERVICE_UV_WARNING_HOURS, &livespan);
            if(res == false){
                ESP_LOGE(TAG, "new hepa warning fault");
                 return false;
            }

            ESP_LOGI(TAG, "new hepa warning %u", livespan);
        }

        return true;
    }

    return false;
}

static bool ReadDeviceUpdate(const char * const jsonBody)
{
    Ota_t ota = {};
    bool parseRes = MessageParserAndSerializerParseDeviceUpdateHttpClientJsonString(jsonBody, &ota);

    if(ota.isAvailable == true){
        OtaCreateTask(&ota);
    }
    
    if((parseRes == true) && (ota.isAvailable == true)){
        ESP_LOGI(TAG, "new FW %s, %X", ota.firmwareUrl, ota.checksum);

        return true;
    }

    return false;
}

static int DeviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback)
{
    ESP_LOGI(TAG, "%s --> %s, %d", method_name, payload, size);

    DirectMessageReturnStatus_t returnStatus = DIRECT_MESSAGE_RETURN_STATUS_ERROR;

    const char methodStatusError[] = "{\"status\":500}";
    const char methodStatusOk[] = "{\"status\":200}";

    bool result = ExecuteReadDirectMessage(payload);
    if(result == true){
        returnStatus = DIRECT_MESSAGE_RETURN_STATUS_OK;
        *response_size = strlen(methodStatusOk);
        *response = malloc(*response_size);
        memcpy(*response, methodStatusOk, *response_size);
    }
    else{
        returnStatus = DIRECT_MESSAGE_RETURN_STATUS_ERROR;
        *response_size = strlen(methodStatusError);
        *response = malloc(*response_size);
        memcpy(*response, methodStatusError, *response_size);
    }
    
    return returnStatus;
}

static bool ProvisioningInit(void)
{
    ESP_LOGI(TAG, "prov start");

    if(IoTHub_Init() != 0){
        return false;
    }

    prov_dev_security_init(SECURE_DEVICE_TYPE_X509);
    
    ESP_LOGI(TAG, "Provisioning API Version: %s", Prov_Device_LL_GetVersionString());
    ESP_LOGI(TAG, "Iothub API Version: %s", IoTHubClient_GetVersionString());

    const char* global_prov_uri = FactorySettingsGetCloudHostName();
    const char* id_scope = FactorySettingsGetScopeIdName();

    ESP_LOGI(TAG, "prow uri: %s", global_prov_uri);
    ESP_LOGI(TAG, "id scope: %s", id_scope);
    ESP_LOGI(TAG, "common name: %s", FactorySettingsGetDevceName());

    PROV_DEVICE_LL_HANDLE provHandle = Prov_Device_LL_Create(global_prov_uri, id_scope, Prov_Device_MQTT_WS_Protocol);
    if (provHandle == NULL){
        ESP_LOGE(TAG, "failed calling Prov_Device_LL_Create");
        prov_dev_security_deinit();
        IoTHub_Deinit();

        sProvInfo.registration_complete = PROV_CONNECTION_STATUS_IDLE;

        return false;
    }

    Prov_Device_LL_SetOption(provHandle, PROV_OPTION_LOG_TRACE, &sTraceOn);

    uint8_t provTimeout = PROV_TIMEOUT_SEC;
    ESP_LOGI(TAG, "Prov timeout %d", provTimeout);
    Prov_Device_LL_SetOption(provHandle, PROV_OPTION_TIMEOUT, &provTimeout);

    if (Prov_Device_LL_Register_Device(provHandle, ProvRegisterDeviceCallback, &sProvInfo, ProvRegistrationStatusCallback, &sProvInfo) != PROV_DEVICE_RESULT_OK){
        ESP_LOGE(TAG, "failed calling Prov_Device_LL_Register_Device");
        prov_dev_security_deinit();
        IoTHub_Deinit();

        sProvInfo.registration_complete = PROV_CONNECTION_STATUS_IDLE;

        return false;
    }

    do{
        Prov_Device_LL_DoWork(provHandle);
        ThreadAPI_Sleep(PROV_SLEEP_TIMIE_MS);
    } while (sProvInfo.registration_complete == PROV_CONNECTION_STATUS_IDLE);

    Prov_Device_LL_Destroy(provHandle);
    ESP_LOGI(TAG, "prov end");

    if(sProvInfo.registration_complete == PROV_HUB_CONNECTION_STATUS_ERROR){
        ESP_LOGE(TAG, "prov error");
        prov_dev_security_deinit();
        IoTHub_Deinit();

        sProvInfo.registration_complete = PROV_CONNECTION_STATUS_IDLE;

        return false;
    }

    SettingSave();

    return true;
}

static void IoTHubClientInit(void)
{
    ESP_LOGI(TAG, "Creating IoTHub Device handle");
    ESP_LOGI(TAG, "uri %s", sProvInfo.iothub_uri);
    ESP_LOGI(TAG, "device id %s", sProvInfo.device_id);

    sIoTHubDeviceHandle = IoTHubDeviceClient_LL_CreateFromDeviceAuth(sProvInfo.iothub_uri, sProvInfo.device_id, MQTT_WebSocket_Protocol);
    if(sIoTHubDeviceHandle == NULL){
        ESP_LOGE(TAG, "failed create IoTHub client from connection string %s", sProvInfo.iothub_uri);
    }

    int keepAliveSecond = IOTHUB_KEEP_ALIVE_SEC;
    ESP_LOGI(TAG, "keep alive %d[S]", keepAliveSecond);

    IoTHubDeviceClient_LL_SetOption(sIoTHubDeviceHandle, OPTION_LOG_TRACE, &sTraceOn);
    IoTHubDeviceClient_LL_SetOption(sIoTHubDeviceHandle, OPTION_KEEP_ALIVE, &keepAliveSecond);
    IoTHubDeviceClient_LL_SetConnectionStatusCallback(sIoTHubDeviceHandle, IotHubConnectionStatus, &sIoTHubInfo);
    IoTHubDeviceClient_LL_SetDeviceMethodCallback(sIoTHubDeviceHandle, DeviceMethodCallback, sIoTHubDeviceHandle);
}

static void IoTHubClientDeInit(void)
{
    ESP_LOGI(TAG, "Iot Hub deinit");

    IoTHubDeviceClient_LL_Destroy(sIoTHubDeviceHandle);
    prov_dev_security_deinit();

    sProvInfo.registration_complete = PROV_CONNECTION_STATUS_IDLE;

    if(sProvInfo.iothub_uri != NULL){
        free(sProvInfo.iothub_uri);
    }

    if(sProvInfo.device_id != NULL){
        free(sProvInfo.device_id);
    }

    IoTHub_Deinit();

    sIoTHubDeviceHandle = NULL;
}

static bool SendSavedDeviceStatus(void)
{
    char jsonStr[MESSAGE_TYPE_MAX_DEVICE_STATUS_JSON_LENGTH] = {};
    messageTypeDeviceStatusHttpClient_t deviceStatus = {};

    uint16_t elementsToSend = PostDataSevingReadSize();
    ESP_LOGI(TAG, "post device status element to send %d", elementsToSend);

    for(uint16_t postIdx = 0; postIdx < elementsToSend; ++postIdx){
        cJSON *jsonRoot = cJSON_CreateObject();
        PostDataSevingRead(&deviceStatus);
        MessageParserAndSerializerCreateDeviceStatusHttpClientJson(jsonRoot, &deviceStatus);

        if (cJSON_PrintPreallocated(jsonRoot, jsonStr, MESSAGE_TYPE_MAX_DEVICE_STATUS_JSON_LENGTH, false) == false) {
            jsonStr[0] = 0;
            ESP_LOGE(TAG, "old device status json size is too big");
            
            return false;
        }

        cJSON_Delete(jsonRoot);

        int jsonStrLen = strlen(jsonStr);

        ESP_LOGI(TAG, "old status %s", jsonStr);

        bool sendStatus = PublishDataEvent(jsonStr, jsonStrLen);
        if(sendStatus == false){
            ESP_LOGW(TAG, "old device stauts websocket send error");
        
            return false;
        }

        ESP_LOGI(TAG, "post device status idx %d send", postIdx);
    }

    PostDataSevingClear();

    return true;
}

static bool IotHubClientSettingLoad(void)
{
    if(xSemaphoreTake(sSettingMutex, MUTEX_TIMEOUT_MS) == pdTRUE){
        iotHubClientStatus_t loadIotHubSetting = {};
        uint16_t loadDataLen = sizeof(iotHubClientStatus_t);

        bool nvsRes = NvsDriverLoad(NVS_KAY_NAME, &loadIotHubSetting, &loadDataLen);
        ESP_LOGI(TAG, "load data len %d", loadDataLen);

        if(nvsRes == true){
            ESP_LOGI(TAG, "load iotHub setting from nvs");
            memcpy(&sIotHubClientStatus, &loadIotHubSetting, sizeof(iotHubClientStatus_t));
        }
        else{
            ESP_LOGI(TAG, "save default iotHub setting nvs");

            memset(&sIotHubClientStatus, 0, sizeof(iotHubClientStatus_t));
            NvsDriverSave(NVS_KAY_NAME, &sIotHubClientStatus, sizeof(iotHubClientStatus_t));
        }
        
        ESP_LOGI(TAG, "load isConnectedLeastOnce: %d from nvs", sIotHubClientStatus.isConnectedLeastOnce);

        xSemaphoreGive(sSettingMutex);

        return nvsRes;
    }

    return false;
}

static bool ExecuteReadDirectMessage(const unsigned char* message)
{
    bool result = false;

    MessageType_t messageType = MessageParserAndSerializerGetMessageType((const char * const)message);
    ESP_LOGI(TAG, "message type %d", messageType);

    switch (messageType)
    {
        case MESSAGE_TYPE_UNKNOWN :
        {
            ESP_LOGE(TAG, "message type unknown %s", message);
            break;
        }
        case MESSAGE_TYPE_DEVICE_LOCATION :
        {
            ESP_LOGI(TAG, "read device location");
            Location_t location = {};
            LocationGet(&location);
            result = ReadDeviceLocation((const char * const)message, &location);
            break;
        }
        case MESSAGE_TYPE_DEVICE_SCHEDULE :
        {
            ESP_LOGI(TAG, "read device schedule");
            Scheduler_t scheduler = {};
            SchedulerGetAll(&scheduler);
            result = ReadDeviceScheduler((const char * const)message, &scheduler);
            break;
        }
        case MESSAGE_TYPE_DEVICE_MODE :
        {
            ESP_LOGI(TAG, "read device mode");
            SettingDevice_t setting = {};
            SettingGet(&setting);
            result = ReadDeviceMode((const char * const)message, &setting);
            break;
        }
        case MESSAGE_TYPE_DEVICE_SERVICE :
        {
            ESP_LOGI(TAG, "read device service");
            result = ReadDeviceService((const char * const)message);
            break;
        }
        case MESSAGE_TYPE_DEVICE_UPDATE :
        {
            ESP_LOGI(TAG, "read device update");
            result = ReadDeviceUpdate((const char * const)message);
            break;
        }
        default:
        {
            ESP_LOGW(TAG, "wrong message type");
            break;
        }
    }

    return result;
}