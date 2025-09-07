/**
 * @file messageParserAndSerializer.c
 *
 * @brief message parser & serializere source file
 *
 * @dir webServer
 * @brief webServer middleware folder
 *
 * @author matfio
 * @date 2021.09.01
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "messageParserAndSerializer.h"

#include <esp_log.h>

#include "nvsDriver/nvsDriver.h"
#include "wifi/wifi.h"
#include "factorySettingsDriver/factorySettingsDriver.h"

#include <string.h>

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
*****************************************************************************/

#define TEMP_DATE_TIME_BUFFER_SIZE (64U)

#define CLAER_HEPA_COUNTER_STRING           ("HEPA")
#define CLAER_UV_LAMP_1_COUNTER_STRING      ("UV1")
#define CLAER_UV_LAMP_2_COUNTER_STRING      ("UV2")

#define DEVICE_AUTH_TYPE_SERVICE_STRING     ("TIMER")
#define DEVICE_AUTH_TYPE_DIAGNOSTIC_STRING  ("DIAG")

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static const char *sMessageTypeNameStr[MESSAGE_TYPE_DEVICE_COUNT] = {
    [MESSAGE_TYPE_DEVICE_INFO]          = "deviceInfo",
    [MESSAGE_TYPE_DEVICE_STATUS]        = "deviceStatus",
    [MESSAGE_TYPE_DEVICE_LOCATION]      = "deviceLocation",
    [MESSAGE_TYPE_DEVICE_SCHEDULE]      = "deviceSchedule",
    [MESSAGE_TYPE_DEVICE_MODE]          = "deviceMode",
    [MESSAGE_TYPE_DEVICE_SERVICE]       = "deviceService",
    [MESSAGE_TYPE_DEVICE_UPDATE]        = "deviceUpdate"
};

static const char *sEapMethodStr[WIFI_EAP_METHOD_COUNT] = {
    [WIFI_EAP_METHOD_NONE]              = "",
    [WIFI_EAP_METHOD_TLS]               = "TLS",
    [WIFI_EAP_METHOD_PEAP]              = "PEAP",
    [WIFI_EAP_METHOD_TTLS]              = "TTLS"
};

static const char *sEapPhase2MethodStr[ESP_EAP_TTLS_PHASE2_CHAP + 1] = {
    [ESP_EAP_TTLS_PHASE2_EAP]       = "EAP",
    [ESP_EAP_TTLS_PHASE2_MSCHAPV2]  = "MSCHAPV2",
    [ESP_EAP_TTLS_PHASE2_MSCHAP]    = "MSCHAP",
    [ESP_EAP_TTLS_PHASE2_PAP]       = "PAP",
    [ESP_EAP_TTLS_PHASE2_CHAP]      = "CHAP"
};

static const char *TAG = "m_parser_seria";

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Check if recaive device is the same as on the device
 *  @param id string to compare
 *  @return true when yes
 */
static bool IsDeviceIdCorrect(const char* id);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

MessageType_t MessageParserAndSerializerGetMessageType(const char * const deviceTypeBody)
{
    cJSON *deviceNameJson = cJSON_Parse(deviceTypeBody);
    if (deviceNameJson == NULL){
        ESP_LOGW(TAG, "message name parse error");

        cJSON_Delete(deviceNameJson);
        return MESSAGE_TYPE_UNKNOWN;
    }

    const cJSON *Name = cJSON_GetObjectItemCaseSensitive(deviceNameJson, "MessageName");
    if(cJSON_IsString(Name) == false){
        ESP_LOGW(TAG, "message name is not string");

        cJSON_Delete(deviceNameJson);
        return MESSAGE_TYPE_UNKNOWN;
    }

    uint16_t nameLen = strlen(Name->valuestring);
    for(uint16_t idx = 0; idx < MESSAGE_TYPE_DEVICE_COUNT; ++idx)
    {
        uint16_t messageTypeNameLen = strlen(sMessageTypeNameStr[idx]);
        if((nameLen == messageTypeNameLen) && (strncmp(Name->valuestring, sMessageTypeNameStr[idx], nameLen) == 0)){
            ESP_LOGI(TAG, "message name %d", idx);

            cJSON_Delete(deviceNameJson);
            return idx;
        }
    }

    ESP_LOGW(TAG, "unknown message name");

    cJSON_Delete(deviceNameJson);
    return MESSAGE_TYPE_UNKNOWN;
}

bool MessageParserAndSerializerCreateDeviceInfoJson(cJSON *root, const messageTypeDeviceInfo_t* deviceInfo)
{
    cJSON_AddNumberToObject(root, "fan", deviceInfo->fan);
    if (deviceInfo->Switch == true){
        cJSON_AddTrueToObject(root, "switch");
    }
    else{
        cJSON_AddFalseToObject(root, "switch");
    }
    cJSON_AddNumberToObject(root, "lamp", deviceInfo->lamp);
    cJSON_AddNumberToObject(root, "hepa", deviceInfo->hepa);
    cJSON_AddStringToObject(root, "sw_version",deviceInfo->sw_version);
    cJSON_AddStringToObject(root, "compile_date",deviceInfo->compile_date);
    cJSON_AddStringToObject(root, "compile_time",deviceInfo->compile_time);
    if (deviceInfo->automatical == true){
        cJSON_AddTrueToObject(root, "automatical");
    }
    else{
        cJSON_AddFalseToObject(root, "automatical");
    }

    if (deviceInfo->wifiConnect == true){
        cJSON_AddTrueToObject(root, "wifi");
    }
    else{
        cJSON_AddFalseToObject(root, "wifi");
    }

    if (deviceInfo->ecoOn == true){
        cJSON_AddTrueToObject(root, "ecomode");
    }
    else{
        cJSON_AddFalseToObject(root, "ecomode");
    }

    if (deviceInfo->lockOn == true){
        cJSON_AddTrueToObject(root, "touchLock");
    }
    else{
        cJSON_AddFalseToObject(root, "touchLock");
    }

    cJSON_AddNumberToObject(root, "Timestamp", deviceInfo->timestamp);
    
    return true;
}

bool MessageParserAndSerializerParseDeviceModeJsonString(const char * const deviceModeBody, messageTypeDeviceMode_t* deviceMode)
{
    cJSON *deviceModeJson = cJSON_Parse(deviceModeBody);
    if (deviceModeJson == NULL){
		 ESP_LOGE( TAG, "fails parsing JSON" );
        cJSON_Delete(deviceModeJson);
        return false;
    }

    const cJSON *Switch = cJSON_GetObjectItemCaseSensitive(deviceModeJson, "switch");
    if (cJSON_IsBool(Switch))
    {
        deviceMode->Switch = Switch->valueint;
    }

    const cJSON *fan = cJSON_GetObjectItemCaseSensitive(deviceModeJson, "fan");
    if (cJSON_IsNumber(fan))
    {
        deviceMode->fan = fan->valueint;
    }

    const cJSON *automatical = cJSON_GetObjectItemCaseSensitive(deviceModeJson, "automatical");
    if (cJSON_IsBool(automatical))
    {
        deviceMode->automatical = automatical->valueint;
    }

    const cJSON *wifi = cJSON_GetObjectItemCaseSensitive(deviceModeJson, "wifi");
    if (cJSON_IsBool(wifi))
    {
        deviceMode->wifiConnect = wifi->valueint;
    }

    const cJSON *lock = cJSON_GetObjectItemCaseSensitive(deviceModeJson, "touchLock");
    if (cJSON_IsBool(lock))
    {
        deviceMode->lockOn = lock->valueint;
    }

    const cJSON *eco = cJSON_GetObjectItemCaseSensitive(deviceModeJson, "ecomode");
    if (cJSON_IsBool(eco))
    {
        deviceMode->ecoOn = eco->valueint;
    }

    cJSON_Delete(deviceModeJson);
    
    return true;
}

bool MessageParserAndSerializerParseDeviceAuthJsonString(const char * const deviceAuthBody, messageTypeDeviceAuthType_t* deviceAuth)
{
    *deviceAuth  = MESSAGE_TYPE_AUTH_TYPE_FAIL;

    cJSON *deviceAuthJson = cJSON_Parse(deviceAuthBody);
    if (deviceAuthJson == NULL){
		ESP_LOGE( TAG, "fails parsing JSON" );
        cJSON_Delete(deviceAuthJson);
        return false;
    }

    const cJSON *type = cJSON_GetObjectItemCaseSensitive(deviceAuthJson, "type");
    const cJSON *password = cJSON_GetObjectItemCaseSensitive(deviceAuthJson, "password");

    if((cJSON_IsString(type) == false) || (cJSON_IsString(password) == false)){
        cJSON_Delete(deviceAuthJson);
        return true;
    }

    int typeCmp = strncmp(type->valuestring, DEVICE_AUTH_TYPE_SERVICE_STRING, strlen(DEVICE_AUTH_TYPE_SERVICE_STRING));
    if(typeCmp == 0){
        char* servicePass = FactorySettingsGetServicePassword();
        if(servicePass != NULL){
            int servicePasswordLenReadFactory = strlen(servicePass);
            int servicePasswordLenFromPost = strlen(password->valuestring);

            if(servicePasswordLenReadFactory == servicePasswordLenFromPost){
                int passCmp = strncmp(password->valuestring, servicePass, servicePasswordLenFromPost);
                if(passCmp == 0){
                    *deviceAuth = MESSAGE_TYPE_AUTH_TYPE_SERVICE;
                }
            }
        }
    }

    typeCmp = strncmp(type->valuestring, DEVICE_AUTH_TYPE_DIAGNOSTIC_STRING, strlen(DEVICE_AUTH_TYPE_DIAGNOSTIC_STRING));
    if(typeCmp == 0){
        char* servicePass = FactorySettingsGetDiagnosticPassword();
        if(servicePass != NULL){
            int servicePasswordLenReadFactory = strlen(servicePass);
            int servicePasswordLenFromPost = strlen(password->valuestring);

            if(servicePasswordLenReadFactory == servicePasswordLenFromPost){
                int passCmp = strncmp(password->valuestring, servicePass, servicePasswordLenFromPost);
                if(passCmp == 0){
                    *deviceAuth = MESSAGE_TYPE_AUTH_TYPE_DIAGNOSTIC;
                }
            }
        }
    }

    cJSON_Delete(deviceAuthJson);
    
    return true;
}

bool MessageParserAndSerializerParseDeviceSchedulerJsonString(const char * const deviceSchedulerBody, messageTypeScheduler_t* scheduler)
{
    cJSON *schdulerJson = cJSON_Parse(deviceSchedulerBody);
    if (schdulerJson == NULL){
        cJSON_Delete(schdulerJson);

        return false;
    }

    const cJSON *DeviceId = cJSON_GetObjectItemCaseSensitive(schdulerJson, "DeviceId");
    if (cJSON_IsString(DeviceId))
    {
        ESP_LOGI(TAG, "device id %s",DeviceId->valuestring);
    }

    const cJSON *timestamp = cJSON_GetObjectItemCaseSensitive(schdulerJson, "Timestamp");
    if (cJSON_IsNumber(timestamp))
    {
        ESP_LOGI(TAG, "timestamp %u", timestamp->valueint);

        scheduler->timestamp = timestamp->valueint;
    }

    for(uint16_t dayIdx = 0; dayIdx < SCHEDULER_DAY_COUNT; ++dayIdx){
        const char* dayStr = SchedulerGetStringDayName(dayIdx);
        if(dayStr == NULL){
            ESP_LOGE(TAG, "unknown day");
            cJSON_Delete(schdulerJson);

            return false;
        }

        const cJSON *dayArray = cJSON_GetObjectItemCaseSensitive(schdulerJson, dayStr);
        if(cJSON_IsObject(dayArray) == false){
            ESP_LOGW(TAG, "json does not contain a day %s", dayStr);
            continue;
        }

        const cJSON *fan = cJSON_GetObjectItemCaseSensitive(dayArray, "fan");
        const cJSON *eco = cJSON_GetObjectItemCaseSensitive(dayArray, "eco");
        if((cJSON_IsArray(fan) == false) && (cJSON_GetArraySize(fan) != SCHEDULER_HOUR_COUNT) && 
        (cJSON_IsArray(eco) == false) && (cJSON_GetArraySize(eco) != SCHEDULER_HOUR_COUNT)){
            cJSON_Delete(schdulerJson);

            return false;
        }

        for(uint16_t hourIdx = 0; hourIdx < SCHEDULER_HOUR_COUNT; ++hourIdx){
            const cJSON *settings = cJSON_GetArrayItem(fan, hourIdx);
            const cJSON *ecoModes = cJSON_GetArrayItem(eco, hourIdx);

            if(cJSON_IsNumber(settings)){
                int setting = settings->valueint;
                switch (setting)
                {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 5:{
                        scheduler->deviceSetting[dayIdx][hourIdx].setting = setting;
                        break;
                    }
                    default:{
                        ESP_LOGE(TAG, "incorrect value %d", setting);
                        cJSON_Delete(schdulerJson);

                        return false;
                        }
                    }
                    }
                    else{
                        ESP_LOGE(TAG, "json value is not number");
                        cJSON_Delete(schdulerJson);

                        return false;
                    }

                    if(cJSON_IsBool(ecoModes)){
                        scheduler->deviceSetting[dayIdx][hourIdx].isEco = cJSON_IsTrue(ecoModes);
                    }
                    else{
                        ESP_LOGE(TAG, "json value is not bool");
                        cJSON_Delete(schdulerJson);

                        return false;
                    }
            }
        }

    cJSON_Delete(schdulerJson);
    
    return true;
}

bool MessageParserAndSerializerCreateSchedulerJson(cJSON *root, const messageTypeScheduler_t* scheduler)
{
    cJSON_AddStringToObject(root, "MessageName", sMessageTypeNameStr[MESSAGE_TYPE_DEVICE_SCHEDULE]);
    cJSON_AddStringToObject(root, "DeviceId", FactorySettingsGetDevceName());

    cJSON_AddNumberToObject(root, "Timestamp", scheduler->timestamp);

    cJSON *days = NULL;

    cJSON *settings = NULL;
    cJSON *ecoModes = NULL;

    for(uint16_t dayIdx = 0; dayIdx < SCHEDULER_DAY_COUNT; ++dayIdx){
        days = cJSON_CreateObject();
        if (days == NULL){
            return false;
        }

        settings = cJSON_CreateArray();
        if (settings == NULL){
            return false;
        }
        
        ecoModes = cJSON_CreateArray();
        if (ecoModes == NULL){
            return false;
        }

        cJSON_AddItemToObject(root, SchedulerGetStringDayName(dayIdx), days);

        cJSON_AddItemToObject(days, "fan", settings);
        cJSON_AddItemToObject(days, "eco", ecoModes);

        for(uint16_t hourIdx = 0; hourIdx < SCHEDULER_HOUR_COUNT; ++hourIdx){
            cJSON *setting = cJSON_CreateNumber(scheduler->deviceSetting[dayIdx][hourIdx].setting);
            if(setting == NULL){
                return false;
            }
            cJSON_AddItemToArray(settings, setting);
        }

        for(uint16_t hourIdx = 0; hourIdx < SCHEDULER_HOUR_COUNT; ++hourIdx){
            cJSON *ecoMode = NULL;

            if(scheduler->deviceSetting[dayIdx][hourIdx].isEco == true){
                ecoMode = cJSON_CreateTrue();
            }else{
                ecoMode = cJSON_CreateFalse();
            }

            if (ecoMode == NULL){
                return false;
            }

            cJSON_AddItemToArray(ecoModes, ecoMode);
        }
    }
    
    return true;
}

bool MessageParserAndSerializerParseWifiSettingJsonString(const char * const wifiSettingBody, wifiSetting_t* wifiSetting)
{
    cJSON *wifiJson = cJSON_Parse(wifiSettingBody);
    if (wifiJson == NULL){
        cJSON_Delete(wifiJson);
        return false;
    }

    const cJSON *DeviceId = cJSON_GetObjectItemCaseSensitive(wifiJson, "DeviceId");
    if (cJSON_IsString(DeviceId))
    {
        ESP_LOGI(TAG, "device id %s", DeviceId->valuestring);
    }

    const cJSON *ssid = cJSON_GetObjectItemCaseSensitive(wifiJson, "SSID");
    if (cJSON_IsString(ssid))
    {
        int len = strlen(ssid->valuestring);
        
        if(len < WIFI_SSID_STRING_NAME_LEN){
            memcpy(wifiSetting->ssid, ssid->valuestring, len);
        }
        else{
            ESP_LOGE(TAG, "ssid incorrect size");
            cJSON_Delete(wifiJson);
    
            return false;
        }
    }

    const cJSON *password = cJSON_GetObjectItemCaseSensitive(wifiJson, "Password");
    if (cJSON_IsString(password))
    {
        int len = strlen(password->valuestring);

        if(len < WIFI_PASSWORD_STRING_NAME_LEN){
            memcpy(wifiSetting->password, password->valuestring, len);
        }
        else{
            ESP_LOGE(TAG, "password incorrect size");
            cJSON_Delete(wifiJson);
    
            return false;
        }
    }

    wifiSetting->eapMethod = WIFI_EAP_METHOD_NONE;
    const cJSON *eapMethod = cJSON_GetObjectItemCaseSensitive(wifiJson, "eapMethod");
    if(cJSON_IsString(eapMethod))
    {
        for(uint16_t idx = WIFI_EAP_METHOD_TLS; idx < WIFI_EAP_METHOD_COUNT; ++idx)
        {
            int cmpRes = strncmp(eapMethod->valuestring, sEapMethodStr[idx], strlen(sEapMethodStr[idx]));
            if(cmpRes == 0){
                wifiSetting->eapMethod = idx;
                break;
            }
        }
    }

    ESP_LOGI(TAG, "EAP method %d", wifiSetting->eapMethod);

    const cJSON *radius = cJSON_GetObjectItemCaseSensitive(wifiJson, "radius");
    if (cJSON_IsString(radius))
    {
        int len = strlen(radius->valuestring);

        if(len < WIFI_RADIUS_SERVER_ADDRESS_LEN){
            memcpy(wifiSetting->radiusServerAddress, radius->valuestring, len);
            ESP_LOGI(TAG, "radius addres len %d", len);
        }
        else{
            ESP_LOGE(TAG, "radius incorrect size");
            cJSON_Delete(wifiJson);
    
            return false;
        }
    }

    const cJSON *pem = cJSON_GetObjectItemCaseSensitive(wifiJson, "pem");
    if(cJSON_IsString(pem))
    {
        int len = strlen(pem->valuestring);
        if(len > 0){
            wifiSetting->validateServer = true;
            NvsDriverSave(WIFI_WPA2_CA_PEM_FILE_NAME, pem->valuestring, len);
            ESP_LOGI(TAG, "save %d bytes to %s", len, WIFI_WPA2_CA_PEM_FILE_NAME);
        }
    }
    ESP_LOGI(TAG, "pem ca enable %d", wifiSetting->validateServer);

    if(wifiSetting->eapMethod == WIFI_EAP_METHOD_TLS){
        const cJSON *tslCrt = cJSON_GetObjectItemCaseSensitive(wifiJson, "crt");
        if(cJSON_IsString(tslCrt))
        {
            int len = strlen(tslCrt->valuestring);
            if(len > 0){
                NvsDriverSave(WIFI_WPA2_CLIENT_CRT_FILE_NAME, tslCrt->valuestring, len);
                ESP_LOGI(TAG, "save %d bytes to %s", len, WIFI_WPA2_CLIENT_CRT_FILE_NAME);
            }
        }

        const cJSON *tslKey = cJSON_GetObjectItemCaseSensitive(wifiJson, "key");
        if(cJSON_IsString(tslKey))
        {
            int len = strlen(tslKey->valuestring);
            if(len > 0){
                NvsDriverSave(WIFI_WPA2_CLIENT_KEY_FILE_NAME, tslKey->valuestring, len);
                ESP_LOGI(TAG, "save %d bytes to %s", len, WIFI_WPA2_CLIENT_KEY_FILE_NAME);
            }
        }
    }
    else if((wifiSetting->eapMethod == WIFI_EAP_METHOD_PEAP) || (wifiSetting->eapMethod == WIFI_EAP_METHOD_TTLS)){
        const cJSON *eapUser = cJSON_GetObjectItemCaseSensitive(wifiJson, "eapuser");
        if(cJSON_IsString(eapUser))
        {
            int len = strlen(eapUser->valuestring);
            if(len < WIFI_PEAP_USEN_LEN){
                memcpy(wifiSetting->wpa2PeapEapUser, eapUser->valuestring, len);
                ESP_LOGI(TAG, "eap user len %d", len);
            }
            else{
                ESP_LOGE(TAG, "peap user incorrect size");
                cJSON_Delete(wifiJson);
    
                return false;
            }
        }

        const cJSON *eapPassword = cJSON_GetObjectItemCaseSensitive(wifiJson, "eappassword");
        if(cJSON_IsString(eapPassword))
        {
            int len = strlen(eapPassword->valuestring);
            if(len < WIFI_PEAP_USEN_LEN){
                memcpy(wifiSetting->wpa2PeapPassword, eapPassword->valuestring, len);
                ESP_LOGI(TAG, "eap password len %d", len);
            }
            else{
                ESP_LOGE(TAG, "peap password incorrect size");
                cJSON_Delete(wifiJson);
    
                return false;
            }
        }

        if(wifiSetting->eapMethod == WIFI_EAP_METHOD_TTLS){
            wifiSetting->phase2Method = ESP_EAP_TTLS_PHASE2_EAP;
            const cJSON *phase2Method = cJSON_GetObjectItemCaseSensitive(wifiJson, "phase2Method");
            if(cJSON_IsString(phase2Method))
            {
                for(uint16_t idx = 0; idx <= ESP_EAP_TTLS_PHASE2_CHAP; ++idx)
                {
                    int cmpRes = strncmp(phase2Method->valuestring, sEapPhase2MethodStr[idx], strlen(sEapPhase2MethodStr[idx]));
                    if(cmpRes == 0){
                        wifiSetting->phase2Method = idx;
                        break;
                    }
                }
            }
            ESP_LOGI(TAG, "phase2Method %d", wifiSetting->phase2Method);
        }
    }

    cJSON_Delete(wifiJson);
    
    ESP_LOGI(TAG, "new ssid and passord save");

    return true;
}

bool MessageParserAndSerializerCreateDeviceInfoHttpClientJson(cJSON *root, const messageTypeDeviceInfoHttpClient_t* deviceInfo)
{
    cJSON_AddStringToObject(root, "MessageName",sMessageTypeNameStr[MESSAGE_TYPE_DEVICE_INFO]);
    cJSON_AddStringToObject(root, "DeviceId", FactorySettingsGetDevceName());

    cJSON_AddStringToObject(root, "HwVersion", deviceInfo->hwVersion);
    cJSON_AddStringToObject(root, "FwVersion", deviceInfo->swVersion);

    return true;
}

bool MessageParserAndSerializerCreateDeviceStatusHttpClientJson(cJSON *root, const messageTypeDeviceStatusHttpClient_t* deviceStatus)
{
    cJSON_AddStringToObject(root, "MessageName",sMessageTypeNameStr[MESSAGE_TYPE_DEVICE_STATUS]);
    cJSON_AddStringToObject(root, "DeviceId", FactorySettingsGetDevceName());

    cJSON_AddNumberToObject(root, "Timestamp", deviceStatus->timestamp);
    cJSON_AddNumberToObject(root, "DeviceMode", deviceStatus->mode);
    cJSON_AddNumberToObject(root, "TotalOn", deviceStatus->totalOn);
    cJSON_AddNumberToObject(root, "TimUv1", deviceStatus->timUv1);
    cJSON_AddNumberToObject(root, "TimUv2", deviceStatus->timUv2);
    cJSON_AddNumberToObject(root, "TimHepa", deviceStatus->timHepa);
    cJSON_AddNumberToObject(root, "Rtc", deviceStatus->rtc);
    cJSON_AddNumberToObject(root, "FanLevel", deviceStatus->fanLevel);
    cJSON_AddBoolToObject(root, "EcoMode", deviceStatus->isEco);

    if(deviceStatus->alarmCodeIdx != 0){
        cJSON *codes = cJSON_CreateArray();
        if (codes == NULL){
            return false;
        }
        
        for(uint16_t codeIdx = 0; codeIdx < deviceStatus->alarmCodeIdx; ++codeIdx){
            cJSON *code = cJSON_CreateNumber(deviceStatus->alarmCode[codeIdx]);
            if(code == NULL){
                return false;
            }
            cJSON_AddItemToArray(codes, code);
        }

        cJSON_AddItemToObject(root, "AlarmCodes", codes);
    }
    
    cJSON_AddBoolToObject(root, "EthernetOn", deviceStatus->ethernetOn);
    cJSON_AddBoolToObject(root, "TouchLock", deviceStatus->touchLock);
    cJSON_AddBoolToObject(root, "WifiOn", deviceStatus->wifiOn);
    cJSON_AddBoolToObject(root, "DeviceReset", deviceStatus->deviceReset);
    cJSON_AddNumberToObject(root, "ResetReason", deviceStatus->resetReason);

    return true;
}

bool MessageParserAndSerializerCreateDeviceLocationHttpClientJson(cJSON *root, const Location_t* deviceLocation)
{
    cJSON_AddStringToObject(root, "MessageName",sMessageTypeNameStr[MESSAGE_TYPE_DEVICE_LOCATION]);
    cJSON_AddStringToObject(root, "DeviceId", FactorySettingsGetDevceName());
    
    cJSON_AddStringToObject(root, "Location", deviceLocation->address);
    cJSON_AddStringToObject(root, "Room", deviceLocation->room);

    return true;
}

bool MessageParserAndSerializerParseDeviceLocationHttpClientJsonString(const char * const deviceLocationBody, Location_t* location)
{
    cJSON *deviceLocationJson = cJSON_Parse(deviceLocationBody);
    if (deviceLocationJson == NULL){
        cJSON_Delete(deviceLocationJson);
        return false;
    }

    const cJSON *deviceId = cJSON_GetObjectItemCaseSensitive(deviceLocationJson, "DeviceId");
    if (cJSON_IsString(deviceId))
    {
        if(IsDeviceIdCorrect(deviceId->valuestring) == false){
            ESP_LOGE(TAG, "incorrect deviceId");

            cJSON_Delete(deviceLocationJson);
            return false;
        }
    }

    const cJSON *address = cJSON_GetObjectItemCaseSensitive(deviceLocationJson, "Location");
    if (cJSON_IsString(address))
    {   
        int len = strlen(address->valuestring);
        
        if(len >= LOCATION_ADDRESS_NAME_STRING_LEN){
            ESP_LOGE(TAG, "to long location address string");
        }
        else{
            memset(location->address, 0, LOCATION_ADDRESS_NAME_STRING_LEN);
            memcpy(location->address, address->valuestring, len);
        }
    }

    const cJSON *room = cJSON_GetObjectItemCaseSensitive(deviceLocationJson, "Room");
    if (cJSON_IsString(room))
    {
        int len = strlen(room->valuestring);

        if(len >= LOCATION_ROOM_NAME_STRING_LEN){
            ESP_LOGE(TAG, "to long location room string");
        }else{
            memset(location->room, 0, LOCATION_ROOM_NAME_STRING_LEN);
            memcpy(location->room, room->valuestring, len);
        }
    }

    cJSON_Delete(deviceLocationJson);
    
    return true;
}

bool MessageParserAndSerializerParseDeviceModeHttpClientJsonString(const char * const deviceModeBody, messageTypeDeviceModeHttpClient_t* deviceMode)
{
    cJSON *deviceModeJson = cJSON_Parse(deviceModeBody);
    if (deviceModeJson == NULL){
        cJSON_Delete(deviceModeJson);
        return false;
    }

    const cJSON *deviceId = cJSON_GetObjectItemCaseSensitive(deviceModeJson, "DeviceId");
    if (cJSON_IsString(deviceId))
    {
        if(IsDeviceIdCorrect(deviceId->valuestring) == false){
            ESP_LOGE(TAG, "incorrect deviceId");

            cJSON_Delete(deviceModeJson);
            return false;
        }
    }

    const cJSON *mode = cJSON_GetObjectItemCaseSensitive(deviceModeJson, "DeviceMode");
    if (cJSON_IsNumber(mode))
    {   
        deviceMode->mode = mode->valueint;
    }

    const cJSON *fan = cJSON_GetObjectItemCaseSensitive(deviceModeJson, "FanLevel");
    if (cJSON_IsNumber(fan))
    {   
        deviceMode->fanLevel = fan->valueint;
    }

    const cJSON *eco = cJSON_GetObjectItemCaseSensitive(deviceModeJson, "EcoMode");
    if (cJSON_IsBool(eco))
    {   
        deviceMode->isEco = cJSON_IsTrue(eco);
    }

    const cJSON *touch = cJSON_GetObjectItemCaseSensitive(deviceModeJson, "TouchLock");
    if (cJSON_IsBool(touch))
    {   
        deviceMode->touchLock = cJSON_IsTrue(touch);
    }

    cJSON_Delete(deviceModeJson);

    return true;
}

bool MessageParserAndSerializerParseDeviceServiceHttpClientJsonString(const char * const deviceServiceBody, messageTypeDeviceServiceHttpClient_t* deviceService)
{
    cJSON *deviceServiceJson = cJSON_Parse(deviceServiceBody);
    if (deviceServiceJson == NULL){
        cJSON_Delete(deviceServiceJson);
        return false;
    }

    const cJSON *deviceId = cJSON_GetObjectItemCaseSensitive(deviceServiceJson, "DeviceId");
    if (cJSON_IsString(deviceId))
    {
        if(IsDeviceIdCorrect(deviceId->valuestring) == false){
            ESP_LOGE(TAG, "incorrect deviceId");

            cJSON_Delete(deviceServiceJson);
            return false;
        }
    }

    const cJSON *reset = cJSON_GetObjectItemCaseSensitive(deviceServiceJson, "DeviceReset");
    if (cJSON_IsNumber(reset))
    {   
        deviceService->deviceReset = reset->valueint;
    }

    const cJSON *uv1Reload = cJSON_GetObjectItemCaseSensitive(deviceServiceJson, "TimUv1Reload");
    if (cJSON_IsNumber(uv1Reload))
    {   
        deviceService->uv1TimerReload = uv1Reload->valueint;
    }

    const cJSON *uv2Reload = cJSON_GetObjectItemCaseSensitive(deviceServiceJson, "TimUv2Reload");
    if (cJSON_IsNumber(uv2Reload))
    {   
        deviceService->uv2TimerReload = uv2Reload->valueint;
    }

    const cJSON *hepaReload = cJSON_GetObjectItemCaseSensitive(deviceServiceJson, "TimHepaReload");
    if (cJSON_IsNumber(hepaReload))
    {   
        deviceService->hepaTimerReload = hepaReload->valueint;
    }

    const cJSON *scheduleReset = cJSON_GetObjectItemCaseSensitive(deviceServiceJson, "ScheduleReset");
    if (cJSON_IsNumber(scheduleReset))
    {   
        deviceService->scheduleReset = scheduleReset->valueint;
    }

    const cJSON *rtc = cJSON_GetObjectItemCaseSensitive(deviceServiceJson, "RtcSet");
    if (cJSON_IsNumber(rtc))
    {   
        deviceService->rtcTime = (uint32_t)rtc->valuedouble;
        deviceService->rtcTimeIsSet = true;
    }

    const cJSON *hepaLivespan = cJSON_GetObjectItemCaseSensitive(deviceServiceJson, "HepaLivespan");
    if (cJSON_IsNumber(hepaLivespan))
    {   
        deviceService->hepaLivespan = hepaLivespan->valueint;
        deviceService->hepaLivespanIsSet = true;
    }

    const cJSON *hepaWarning = cJSON_GetObjectItemCaseSensitive(deviceServiceJson, "HepaWarning");
    if (cJSON_IsNumber(hepaWarning))
    {   
        deviceService->hepaWarning = hepaWarning->valueint;
        deviceService->hepaWarningIsSet = true;
    }

    const cJSON *uvLivespan = cJSON_GetObjectItemCaseSensitive(deviceServiceJson, "UvLivespan");
    if (cJSON_IsNumber(uvLivespan))
    {   
        deviceService->uvLivespan = uvLivespan->valueint;
        deviceService->uvLivespanIsSet = true;
    }

    const cJSON *uvWarning = cJSON_GetObjectItemCaseSensitive(deviceServiceJson, "UvWarning");
    if (cJSON_IsNumber(uvWarning))
    {   
        deviceService->uvWarning = uvWarning->valueint;
        deviceService->uvWarningIsSet = true;
    }

    const cJSON *utcTimeOffset = cJSON_GetObjectItemCaseSensitive(deviceServiceJson, "UtcTimeoffset");
    if (cJSON_IsNumber(utcTimeOffset))
    {   
        deviceService->utcTimeOffset = utcTimeOffset->valuedouble;
        deviceService->utcTimeOffsetIsSet = true;
    }
    
    cJSON_Delete(deviceServiceJson);

    return true;
}

bool MessageParserAndSerializerParseDeviceUpdateHttpClientJsonString(const char * const deviceUpdateBody, Ota_t* ota)
{
    cJSON *deviceUpdateJson = cJSON_Parse(deviceUpdateBody);
    if (deviceUpdateJson == NULL){
        cJSON_Delete(deviceUpdateJson);
        return false;
    }

    const cJSON *deviceId = cJSON_GetObjectItemCaseSensitive(deviceUpdateJson, "DeviceId");
    if (cJSON_IsString(deviceId))
    {
        if(IsDeviceIdCorrect(deviceId->valuestring) == false){
            ESP_LOGE(TAG, "incorrect deviceId");

            cJSON_Delete(deviceUpdateJson);
            return false;
        }
    }

    bool urlIsSet = false;
    bool checksumIsSet = false;
    bool versionIsSet = false;

    const cJSON *newId = cJSON_GetObjectItemCaseSensitive(deviceUpdateJson, "NewDeviceId");
    if (cJSON_IsString(newId))
    {   
        ESP_LOGI(TAG, "old id %s", FactorySettingsGetDevceName());
        bool setRes =  FactorySettingsSetDevceName(newId->valuestring);
        ESP_LOGI(TAG, "new id %s %d", newId->valuestring, setRes);
    }

    const cJSON *fwVersion = cJSON_GetObjectItemCaseSensitive(deviceUpdateJson, "FwVersion");
    if (cJSON_IsString(fwVersion))
    {   
        int len = strlen(fwVersion->valuestring);
        
        if(len >= OTA_NEW_VERSION_STRING_LEN){
            ESP_LOGE(TAG, "to long new firmware version string");
        }
        else{
            OtaFirmwareVersion_t version = {};
            char versionTemp[OTA_NEW_VERSION_STRING_LEN] = {};

            memcpy(versionTemp, fwVersion->valuestring, len);

            if (sscanf(versionTemp, "%u.%u.%u", &version.major, &version.minor, &version.subMinor) != 3) {
                return false;
            }

            memcpy(&ota->version, &version, sizeof(OtaFirmwareVersion_t));

            versionIsSet = true;
        }
    }

    const cJSON *fwUrl = cJSON_GetObjectItemCaseSensitive(deviceUpdateJson, "FwPackageURI");
    if (cJSON_IsString(fwUrl))
    {   
        int len = strlen(fwUrl->valuestring);
        
        if(len >= OTA_NEW_FIRMWARE_URL_STRING_LEN){
            ESP_LOGE(TAG, "to long new firmware url string");
        }
        else{
            memset(ota->firmwareUrl, 0, OTA_NEW_FIRMWARE_URL_STRING_LEN);
            memcpy(ota->firmwareUrl, fwUrl->valuestring, len);
            urlIsSet = true;
        }
    }

    const cJSON *fwChecksum = cJSON_GetObjectItemCaseSensitive(deviceUpdateJson, "FwPackageCheckValue");
    if (cJSON_IsNumber(fwChecksum))
    {
        ota->checksum = (uint32_t)fwChecksum->valuedouble;
        checksumIsSet = true;
    }

    if((urlIsSet == true) && (checksumIsSet == true) && (versionIsSet == true)){
        ota->isAvailable = true;
    }

    return true;
}

bool MessageParserAndSerializerParseDeviceTimeHttpClientJsonString(const char * const deviceTimeBody, struct tm *time, float *offset)
{
    cJSON *deviceTimeJson = cJSON_Parse(deviceTimeBody);
    if (deviceTimeJson == NULL){
        cJSON_Delete(deviceTimeJson);
        return false;
    }

    char dataTimeBuffer[TEMP_DATE_TIME_BUFFER_SIZE] = {};

    int newDateSize = 0;
    int newTimeSize = 0;
    const cJSON *newDate = cJSON_GetObjectItemCaseSensitive(deviceTimeJson, "setDate");
    if (cJSON_IsString(newDate))
    {
        newDateSize = strlen(newDate->valuestring);
    }

    const cJSON *newTime = cJSON_GetObjectItemCaseSensitive(deviceTimeJson, "setTime");
    if (cJSON_IsString(newTime))
    {
        newTimeSize = strlen(newTime->valuestring);
    }

    if((newDateSize == 0) || (newTimeSize == 0))
    {
        ESP_LOGE(TAG, "empty param data or time");
        return false;
    }

    if((newDateSize + newTimeSize + 1 + 1) > TEMP_DATE_TIME_BUFFER_SIZE){ // +1 space char, +1 end char
        ESP_LOGE(TAG, "suspiciously large size of data, time");
        return false;
    }

    strcpy(dataTimeBuffer, newDate->valuestring);
    strcat(dataTimeBuffer, " ");
    strcat(dataTimeBuffer, newTime->valuestring);

    strptime(dataTimeBuffer, "%Y-%m-%d %H:%M:%S", time);

    const cJSON *utcOffset = cJSON_GetObjectItemCaseSensitive(deviceTimeJson, "UtcTimeoffset");
    if (cJSON_IsNumber(utcOffset))
    {
        *offset = utcOffset->valuedouble;
    }

    return true;
}

bool MessageParserAndSerializerParseDeviceCounterHttpClientJsonString(const char * const deviceCounterBody, messageTypeClearCounter_t *counter)
{
    cJSON *deviceCounterJson = cJSON_Parse(deviceCounterBody);
    if (deviceCounterJson == NULL){
        cJSON_Delete(deviceCounterJson);
        return false;
    }

    const cJSON *clearCounter = cJSON_GetObjectItemCaseSensitive(deviceCounterJson, "component");
    if (cJSON_IsString(clearCounter))
    {
        int cmpRes = strncmp(clearCounter->valuestring, CLAER_HEPA_COUNTER_STRING, strlen(CLAER_HEPA_COUNTER_STRING));
        if(cmpRes == 0){
            counter->hepaCounter = true;
        }

        cmpRes = strncmp(clearCounter->valuestring, CLAER_UV_LAMP_1_COUNTER_STRING, strlen(CLAER_UV_LAMP_1_COUNTER_STRING));
        if(cmpRes == 0){
            counter->uvLamp1Counter = true;
        }

        cmpRes = strncmp(clearCounter->valuestring, CLAER_UV_LAMP_2_COUNTER_STRING, strlen(CLAER_UV_LAMP_2_COUNTER_STRING));
        if(cmpRes == 0){
            counter->uvLamp2Counter = true;
        }
    }

    return true;
}

bool MessageParserAndSerializerCreateDeviceDiagnosticJson(cJSON *root, const messageTypeDiagnostic_t* deviceDiag)
{
    if (deviceDiag->hepa1 == true){
        cJSON_AddTrueToObject(root, "hepa1");
    }
    else{
        cJSON_AddFalseToObject(root, "hepa1");
    }

    if (deviceDiag->hepa2 == true){
        cJSON_AddTrueToObject(root, "hepa2");
    }
    else{
        cJSON_AddFalseToObject(root, "hepa2");
    }

    if (deviceDiag->preFilter == true){
        cJSON_AddTrueToObject(root, "prefiltr");
    }
    else{
        cJSON_AddFalseToObject(root, "prefiltr");
    }

    cJSON_AddNumberToObject(root, "balast1", deviceDiag->ballast1Uv);
    cJSON_AddNumberToObject(root, "balast2", deviceDiag->ballast2Uv);

    if (deviceDiag->uv1Relay == true){
        cJSON_AddTrueToObject(root, "Uv1Relay");
    }
    else{
        cJSON_AddFalseToObject(root, "Uv1Relay");
    }

    if (deviceDiag->uv2Relay == true){
        cJSON_AddTrueToObject(root, "Uv2Relay");
    }
    else{
        cJSON_AddFalseToObject(root, "Uv2Relay");
    }

    if (deviceDiag->wifiOn == true){
        cJSON_AddTrueToObject(root, "wifi");
    }
    else{
        cJSON_AddFalseToObject(root, "wifi");
    }

    cJSON_AddNumberToObject(root, "fanIn", deviceDiag->fanSpeed);
    cJSON_AddNumberToObject(root, "fanOut", deviceDiag->fanLevel);

    if (deviceDiag->touchLock == true){
        cJSON_AddTrueToObject(root, "touchLock");
    }
    else{
        cJSON_AddFalseToObject(root, "touchLock");
    }

    cJSON_AddNumberToObject(root, "timerUv1", deviceDiag->timerUv1);
    cJSON_AddNumberToObject(root, "timerUv2", deviceDiag->timerUv2);
    cJSON_AddNumberToObject(root, "timerHepa", deviceDiag->timerHepa);
    cJSON_AddNumberToObject(root, "timerTotal", deviceDiag->timerTotal);

    return true;
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool IsDeviceIdCorrect(const char* id)
{
    return (strncmp(FactorySettingsGetDevceName(), id, strlen(FactorySettingsGetDevceName())) == 0);
}
