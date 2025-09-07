/**
 * @file wifi.c
 *
 * @brief Wifi middleware source file
 *
 * @dir wifi
 * @brief Wifi middleware folder
 *
 * @author matfio
 * @date 2021.11.16
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "wifi/wifi.h"

#include <esp_log.h>

#include <string.h>

#include "esp_wifi.h"
#include "esp_wpa2.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "nvsDriver/nvsDriver.h"
#include "factorySettingsDriver/factorySettingsDriver.h"
#include "gpioExpanderDriver/gpioExpanderDriver.h"
#include "timeDriver/timeDriver.h"

#include "cloud/iotHubClient.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define SETTING_MUTEX_TIMEOUT_MS (1U * 1000U)
#define NVS_KAY_NAME ("wifi")

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static bool initNetif;
static SettingWifiStatus_t sWifiStatus;
static bool sIsWifiRfEmit;

static const char *TAG = "wifi";
static SemaphoreHandle_t sWifiMutex;

static wifiSetting_t sWifiSetting;
static uint32_t sRetryNum;

static int64_t sNewApConnectionTryStart;

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

/** @brief Wi-Fi event handler
 *  @param arg
 *  @param event_base Event base
 *  @param event_id Event ID
 *  @param event_data Event data
 */
static void WifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/** @brief Load wifi setting (ssid, password) from nvs
 *  @return return true if success
 */
static bool WifiSettingLoad(void);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool WifiInit(void)
{
    bool res = true;
    iotHubClientStatus_t websocketSetting = {};
    SettingDevice_t deviceSetting = {};

    res &= SettingGet(&deviceSetting);

    if(initNetif == false){
        // it can only be initiated once
        // there is no function to deinit
        esp_netif_create_default_wifi_ap();
        esp_netif_create_default_wifi_sta();
        initNetif = true;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiEventHandler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiEventHandler, NULL));

    wifi_config_t wifiConfigAp = {
      .ap = {
             .password = CFG_WIFI_AP_PASS,
             .max_connection = CFG_WIFI_AP_CONN,
             .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };

    char* deviceName = FactorySettingsGetDevceName();
    uint16_t deviceNameLen = 0;

    if(deviceName == NULL){
        deviceNameLen = strlen(CFG_WIFI_DEFAULT_AP_SSID);
        memcpy(wifiConfigAp.ap.ssid, CFG_WIFI_DEFAULT_AP_SSID, deviceNameLen);
    }
    else{
        deviceNameLen = strlen(deviceName);
        memcpy(wifiConfigAp.ap.ssid, deviceName, deviceNameLen);
    }

    wifiConfigAp.ap.ssid_len = deviceNameLen;

    if(strlen(CFG_WIFI_AP_PASS) == 0) {
        wifiConfigAp.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifiConfigAp));
    
    ESP_LOGI(TAG, "wifi ap finished. ssid:%s", wifiConfigAp.ap.ssid);
    
    wifi_config_t wifiConfigSta = {
      .sta = {
        .threshold.authmode = WIFI_AUTH_WPA2_PSK}
    };

    if(strlen(sWifiSetting.password) == 0){
        wifiConfigSta.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }

    memcpy(wifiConfigSta.sta.ssid, sWifiSetting.ssid, WIFI_SSID_STRING_NAME_LEN);
    memcpy(wifiConfigSta.sta.password, sWifiSetting.password, WIFI_PASSWORD_STRING_NAME_LEN);

    wifi_mode_t mode = WIFI_MODE_APSTA;

    res &= IotHubClientGetSetting(&websocketSetting);
    if((websocketSetting.isConnectedLeastOnce == true) || (deviceSetting.isConnectNewAp == true)){
        // the correct password and ssid are entered because the device has connected to the cloud at least once
        mode = WIFI_MODE_STA;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfigSta));
    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));

    ESP_LOGI(TAG, "wifi mode %d", mode);
    ESP_LOGI(TAG, "wifi sta finished. set:%d ssid:%s", sWifiSetting.isSet, sWifiSetting.ssid);
    
    ESP_ERROR_CHECK(esp_wifi_start());
    sIsWifiRfEmit = true;

    if(deviceSetting.restore.isWifiOn == false){
        ESP_LOGW(TAG, "wifi is disabled because of wifi switch");
        ESP_ERROR_CHECK(esp_wifi_stop());
        sIsWifiRfEmit = false;
    }
        
    return res;
    
    /*
    if(sWifiSetting.eapMethod == WIFI_EAP_METHOD_NONE){
        ESP_ERROR_CHECK(esp_wifi_start());
        sIsWifiRfEmit = true;

        if(deviceSetting.restore.isWifiOn == false){
            ESP_LOGW(TAG, "wifi is disabled because of wifi switch");
            ESP_ERROR_CHECK(esp_wifi_stop());
            sIsWifiRfEmit = false;
        }
        
        return res;
    }


    ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_identity((const unsigned char *)sWifiSetting.radiusServerAddress, strlen(sWifiSetting.radiusServerAddress)));

    if(sWifiSetting.validateServer == true){
        uint16_t caPemLen = 0;
        res &= NvsDriverLoad(WIFI_WPA2_CA_PEM_FILE_NAME, NULL, &caPemLen);
        
        ESP_LOGI(TAG, "ca pem size %u", caPemLen);
        char* caPemBuffer = (char*) malloc(caPemLen);
        if (caPemBuffer == NULL ){
            return false;
        };

        res &= NvsDriverLoad(WIFI_WPA2_CA_PEM_FILE_NAME, caPemBuffer, &caPemLen);
        
        ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_ca_cert((const unsigned char *)caPemBuffer, caPemLen));
    }

    if(sWifiSetting.eapMethod == WIFI_EAP_METHOD_TLS){
        uint16_t clientCrtLen = 0;
        res &= NvsDriverLoad(WIFI_WPA2_CLIENT_CRT_FILE_NAME, NULL, &clientCrtLen);
        ESP_LOGI(TAG, "client crt size %u", clientCrtLen);

        char* clientCrtBuffer = (char*) malloc(clientCrtLen);
        if (clientCrtBuffer == NULL ){
            return false;
        };

        uint16_t clientPasswordLen = 0;
        res &= NvsDriverLoad(WIFI_WPA2_CLIENT_KEY_FILE_NAME, NULL, &clientPasswordLen);
        ESP_LOGI(TAG, "client password size %u", clientPasswordLen);

        char* clientPasswordBuffer = (char*) malloc(clientPasswordLen);
        if (clientPasswordBuffer == NULL ){
            return false;
        };

        ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_cert_key((const unsigned char *)clientCrtBuffer, clientCrtLen, (const unsigned char *)clientPasswordBuffer, clientPasswordLen, NULL, 0));
    }
    else if((sWifiSetting.eapMethod == WIFI_EAP_METHOD_PEAP) || (sWifiSetting.eapMethod == WIFI_EAP_METHOD_TTLS)){
        ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_username((uint8_t *)sWifiSetting.wpa2PeapEapUser, strlen(sWifiSetting.wpa2PeapEapUser)) );
        ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_password((uint8_t *)sWifiSetting.wpa2PeapPassword, strlen(sWifiSetting.wpa2PeapPassword)) );

        if(sWifiSetting.eapMethod == WIFI_EAP_METHOD_TTLS){
            ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_ttls_phase2_method(sWifiSetting.phase2Method));
        }
    }

    ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_enable());
    ESP_ERROR_CHECK(esp_wifi_start());
    sIsWifiRfEmit = true;

    if(deviceSetting.restore.isWifiOn == false){
        ESP_LOGW(TAG, "wifi is disabled because of wifi switch");
        ESP_ERROR_CHECK(esp_wifi_stop());
        sIsWifiRfEmit = false;
    }
    */
    return res;
}

bool WifiStop(void)
{
    esp_err_t err = esp_wifi_stop();
    
    ESP_LOGI(TAG, "wifi stop %d", err);
    if(err == ESP_OK){
        sIsWifiRfEmit = false;
        return true;
    }

    return false;
}

bool WifiStart(void)
{
    esp_err_t err = esp_wifi_start();
    
    ESP_LOGI(TAG, "wifi start %d", err);
    if(err == ESP_OK){
        sIsWifiRfEmit = true;
        return true;
    }

    return false;
}

bool WifiStaConnect(void)
{
    esp_err_t err = esp_wifi_connect();
    ESP_LOGI(TAG, "wifi connect %d", err);
    if(err == ESP_OK){
        return true;
    }

    return false;
}

bool WifiSettingSave(wifiSetting_t* setting)
{
    if(setting->isSet == true){
        sNewApConnectionTryStart = TimeDriverGetSystemTickMs();
    }

    // new wifi setting ​​we need to connect for the first time again
    iotHubClientStatus_t websocketSetting = {};
    IotHubClientGetSetting(&websocketSetting);
    websocketSetting.isConnectedLeastOnce = false;
    IotHubClientSetSetting(&websocketSetting);

    if (xSemaphoreTake(sWifiMutex, SETTING_MUTEX_TIMEOUT_MS) == pdTRUE) {
        memcpy(&sWifiSetting, setting, sizeof(wifiSetting_t));

        bool res = NvsDriverSave(NVS_KAY_NAME, &sWifiSetting, sizeof(wifiSetting_t));

        xSemaphoreGive(sWifiMutex);
        return res;
    }
    return false;
}

bool WifiSettingGet(wifiSetting_t* setting)
{
    if (xSemaphoreTake(sWifiMutex, SETTING_MUTEX_TIMEOUT_MS) == pdTRUE) {
        memcpy(setting, &sWifiSetting, sizeof(wifiSetting_t));

        xSemaphoreGive(sWifiMutex);
        return true;
    }
    return false;
}

SettingWifiStatus_t WifiGetStaStatus(void)
{
    return sWifiStatus;
}

bool WifiSettingInit(void)
{
    sWifiMutex = xSemaphoreCreateMutex();
    if (sWifiMutex == NULL) {
        return false;
    }

    return WifiSettingLoad();
}

bool WifiReinit(void)
{   
    bool res = true;

    res &= WifiStop();
    esp_err_t err = esp_wifi_deinit();
    if(err != ESP_OK)
    {
        ESP_LOGW(TAG, "esp_wifi_deinit fail");
        res = false;
    }

    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiEventHandler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiEventHandler);

    res &= WifiInit();

    return res;
}

bool WifiRfEmit(void)
{
    return sIsWifiRfEmit;
}

wifi_mode_t WifiModeGet(void)
{
    wifi_mode_t mode = WIFI_MODE_NULL;
    esp_wifi_get_mode(&mode);

    return mode;
}

int64_t WifiGetNewApConnectionTime(void)
{
    return sNewApConnectionTryStart;
}


/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static void WifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if(event_id == WIFI_EVENT_AP_STACONNECTED)
  {
    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
  }
  else if(event_id == WIFI_EVENT_AP_STADISCONNECTED)
  {
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
  }

  if((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_START))
  {
    esp_wifi_connect();
    ESP_LOGI(TAG, "try connect to wifi");
    sWifiStatus = WIFI_STATUS_STA_START;
  }
  else if((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_DISCONNECTED))
  {
    ++sRetryNum;
    ESP_LOGI(TAG,"connect to the AP fail");
    ESP_LOGI(TAG, "retry to connect to the AP %u", sRetryNum);
    
    sWifiStatus = WIFI_STATUS_STA_DISCONNECTED;
  }
  else if((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_STOP))
  {
    ESP_LOGI(TAG,"WIFI STA stop");
    sWifiStatus = WIFI_STATUS_STA_STOP;
  }
  else if((event_base == IP_EVENT) && (event_id == IP_EVENT_STA_GOT_IP))
  {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "wifi got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    sRetryNum = 0;
    sWifiStatus = WIFI_STATUS_STA_CONNECTED;
  }
}

static bool WifiSettingLoad(void)
{
    if (xSemaphoreTake(sWifiMutex, SETTING_MUTEX_TIMEOUT_MS) == pdTRUE) {
        wifiSetting_t loadWifiSetting = {};
        uint16_t loadDataLen = sizeof(wifiSetting_t);
    
        bool nvsRes = NvsDriverLoad(NVS_KAY_NAME, &loadWifiSetting, &loadDataLen);
        ESP_LOGI(TAG, "load data len %d", loadDataLen);

        if(nvsRes == true){
            if(loadDataLen == sizeof(wifiSetting_t)){
                // Load setting OK
                ESP_LOGI(TAG, "load wifi setting from nvs");
                memcpy(&sWifiSetting, &loadWifiSetting, sizeof(wifiSetting_t));
            }else{
                ESP_LOGI(TAG, "read mismatch size");
                NvsDriverSave(NVS_KAY_NAME, &sWifiSetting, sizeof(wifiSetting_t));
            }
        }else{
            ESP_LOGI(TAG, "write default value");
            nvsRes = NvsDriverSave(NVS_KAY_NAME, &sWifiSetting, sizeof(wifiSetting_t));
        }
        
        xSemaphoreGive(sWifiMutex);
        return nvsRes;
    }
    return false;
}