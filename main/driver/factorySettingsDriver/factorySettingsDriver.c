/**
 * @file factorySettingsDriver.c
 *
 * @brief Non-volatile storage driver source file for factory settings
 *
 * @dir factorySettingsDriver
 * @brief NVS driver folder
 *
 * @author matfio
 * @date 2021.10.07
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "factorySettingsDriver.h"
#include "config.h"
#include "setting.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "string.h"

#include "scheduler/scheduler.h"

#include "esp_log.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define PARTITION_NAME "factory_settings"
#define NVS_STORAGE_NAMESPACE "factory_storage"

#define DEVICE_NAME_KAY_NAME "deviceName"
#define DEVICE_NAME_STRING_LEN (CFG_WIFI_AP_SSID_STRING_LEN)

#define DEVICE_TYPE_KAY_NAME "deviceType"
#define DEVICE_TYPE_STRING_LEN (16U)

#define DEVICE_HARDWARE_VERSION_KAY_NAME "hardwareVersion"
#define DEVICE_HARDWARE_VERSION_STRING_LEN (16U)

#define DEVICE_LOCATION_KAY_NAME "location"
#define DEVICE_LOCATION_STRING_LEN (16U)

#define SCHEDULER_KAY_NAME "scheduler"

#define SERVICE_PASSWORD_KAY_NAME "servPass"
#define SERVICE_PASSWORD_STRING_LEN (32U)

#define DIAGNOSTIC_PASSWORD_KAY_NAME "diagnPass"
#define DIAGNOSTIC_PASSWORD_STRING_LEN (32U)

#define HOST_NAME_KAY_NAME "cloudAddress"
#define HOST_NAME_STRING_LEN (128U)

#define ID_SCOPE_KAY_NAME "idScope"
#define ID_SCOPE_STRING_LEN (64U)

#define ROOT_CERT_KAY_NAME "rootCert"
#define INTER_CERT_KAY_NAME "interCert"
#define CLIENT_CERT_KAY_NAME "clientCert"
#define CLIENT_CERT_STRING_LEN (6U * 1024U)

#define CLIENT_KEY_KAY_NAME "clientKey"
#define CLIENT_KEY_STRING_LEN (3828U)

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static uint32_t sServiceParameter[FACTORY_SETTING_SERVICE_COUNT] = {
    [FACTORY_SETTING_SERVICE_HEPA_LIFETIME_HOURS] =     CFG_HEPA_SERVICE_LIFETIME_HOURS,
    [FACTORY_SETTING_SERVICE_HEPA_WARNING_HOURS] =      CFG_HEPA_SERVICE_REPLACEMENT_REMINDER,
    [FACTORY_SETTING_SERVICE_UV_LIFETIME_HOURS] =       CFG_UV_LAMP_SERVICE_LIFETIME_HOURS,
    [FACTORY_SETTING_SERVICE_UV_WARNING_HOURS] =        CFG_UV_LAMP_SERVICE_REPLACEMENT_REMINDER,
    [FACTORY_SETTING_SERVICE_UV_OFF_MIN_MILIVOLT] =     CFG_UV_LAMP_BALAST_OFF_MIN_VOLT_LEVEL,
    [FACTORY_SETTING_SERVICE_UV_OFF_MAX_MILIVOLT] =     CFG_UV_LAMP_BALAST_OFF_MAX_VOLT_LEVEL,
    [FACTORY_SETTING_SERVICE_UV_ON_MIN_MILIVOLT] =      CFG_UV_LAMP_BALAST_ON_MIN_VOLT_LEVEL,
    [FACTORY_SETTING_SERVICE_UV_ON_MAX_MILIVOLT] =      CFG_UV_LAMP_BALAST_ON_MAX_VOLT_LEVEL,
    [FACTORY_SETTING_SERVICE_LOGO_LED_COLOR] =          CFG_TOUCH_DEFAULT_LOGO_COLOR,
    [FACTORY_SETTING_SERVICE_CLOUD_PORT] =              CFG_HTTP_CLIENT_PORT_NUMBER,
};

static const char* TAG = "factory";

static const char* sFanPwmKeyName[FAN_LEVEL_COUNT] = {
    [FAN_LEVEL_1] = "fanPwmLevel1",
    [FAN_LEVEL_2] = "fanPwmLevel2",
    [FAN_LEVEL_3] = "fanPwmLevel3",
    [FAN_LEVEL_4] = "fanPwmLevel4",
    [FAN_LEVEL_5] = "fanPwmLevel5",
};

static const char* sServiceSettingKeyName[FACTORY_SETTING_SERVICE_COUNT] = {
        [FACTORY_SETTING_SERVICE_HEPA_LIFETIME_HOURS] =         "hepaLifeTime",
        [FACTORY_SETTING_SERVICE_HEPA_WARNING_HOURS] =          "hepaWarnTime",
        [FACTORY_SETTING_SERVICE_UV_LIFETIME_HOURS] =           "uvLampLifeTime",
        [FACTORY_SETTING_SERVICE_UV_WARNING_HOURS] =            "uvLampWarnTime",
        [FACTORY_SETTING_SERVICE_UV_OFF_MIN_MILIVOLT] =         "uvLampOffMin",
        [FACTORY_SETTING_SERVICE_UV_OFF_MAX_MILIVOLT] =         "uvLampOffMax",
        [FACTORY_SETTING_SERVICE_UV_ON_MIN_MILIVOLT] =          "uvLampOnMin",
        [FACTORY_SETTING_SERVICE_UV_ON_MAX_MILIVOLT] =          "uvLampOnMax",
        [FACTORY_SETTING_SERVICE_LOGO_LED_COLOR] =              "logoLed",
        [FACTORY_SETTING_SERVICE_CLOUD_PORT] =                  "cloudPort",
};

static char sDeviceNameStr[DEVICE_NAME_STRING_LEN + 1];

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Get string values from actory partition
 *  @param kay [in] to storage
 *  @param raedStr [out] pointer to read string
 *  @param readLen [in] buffer length
 *  @return true if success
 */
static bool GetStringFromPartition(const char* kay, char* raedStr, size_t readLen);

/** @brief Set string values to actory partition
 *  @param kay [in] to storage
 *  @param writeStr [in] pointer to write string
 *  @return true if success
 */
static bool SetStringFromPartition(const char* kay, char* writeStr);

/** @brief Get blob values from actory partition
 *  @param kay [in] to storage
 *  @param raedStr [out] pointer to read string
 *  @param readLen [in] pointer to size
 *  @return true if success
 */
static bool GetBlobFromPartition(const char* kay, char* raedStr, size_t* readLen);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

char* FactorySettingsGetDevceName(void)
{
    if(sDeviceNameStr[0] != 0){
        return sDeviceNameStr;
    }

    if(GetStringFromPartition(DEVICE_NAME_KAY_NAME, sDeviceNameStr, DEVICE_NAME_STRING_LEN) == false){
        sDeviceNameStr[0] = 0;
        return NULL;
    }

    ESP_LOGI(TAG, "Device name %s", sDeviceNameStr);

    return sDeviceNameStr;
}

bool FactorySettingsSetDevceName(char* newDeviceName)
{
    int deviveNameLen = strlen(newDeviceName);
    if(deviveNameLen > DEVICE_NAME_STRING_LEN){
        ESP_LOGW(TAG, "device name to long  %s", newDeviceName);
        return false;
    }

    if(strlen(sDeviceNameStr) == deviveNameLen){
        int cmp = memcmp(newDeviceName, &sDeviceNameStr, deviveNameLen);
        if(cmp == 0){
            // want to save the same value
            return true;
        }
    }

    if(SetStringFromPartition(DEVICE_NAME_KAY_NAME, newDeviceName) == false){
        ESP_LOGW(TAG, "write new name to factory fail");
        return false;
    }

    memset(sDeviceNameStr, 0, (DEVICE_NAME_STRING_LEN + 1));
    memcpy(sDeviceNameStr, newDeviceName, deviveNameLen);

    return true;
}

char* FactorySettingsGetDevceType(void)
{
    static char sDeviceTypeStr[DEVICE_NAME_STRING_LEN + 1] = {};

    if(sDeviceTypeStr[0] != 0){
        return sDeviceTypeStr;
    }

    if(GetStringFromPartition(DEVICE_TYPE_KAY_NAME, sDeviceTypeStr, DEVICE_NAME_STRING_LEN) == false){
        sDeviceTypeStr[0] = 0;
        return NULL;
    }

    ESP_LOGI(TAG, "Device type %s", sDeviceTypeStr);

    return sDeviceTypeStr;
}

char* FactorySettingsGetHardwareVersion(void)
{
    static char sHardwareVersionStr[DEVICE_HARDWARE_VERSION_STRING_LEN + 1] = {};

    if(sHardwareVersionStr[0] != 0){
        return sHardwareVersionStr;
    }
    
    if(GetStringFromPartition(DEVICE_HARDWARE_VERSION_KAY_NAME, sHardwareVersionStr, DEVICE_HARDWARE_VERSION_STRING_LEN) == false){
        sHardwareVersionStr[0] = 0;
        return NULL;
    }

    ESP_LOGI(TAG, "Hardware version %s", sHardwareVersionStr);

    return sHardwareVersionStr;
}

char* FactorySettingsGetLocation(void)
{
    static char sLocationStr[DEVICE_LOCATION_STRING_LEN + 1] = {};

    if(sLocationStr[0] != 0){
        return sLocationStr;
    }
    
    if(GetStringFromPartition(DEVICE_LOCATION_KAY_NAME, sLocationStr, DEVICE_LOCATION_STRING_LEN) == false){
        sLocationStr[0] = 0;
        return NULL;
    }

    ESP_LOGI(TAG, "location %s", sLocationStr);

    return sLocationStr;
}

bool FactorySettingsGetPwmFanLevel(SettingFanLevel_t levelNumber, uint32_t* pwmValue)
{
    static uint32_t sFanPwmValue[FAN_LEVEL_COUNT] = {(0x0fff / 5), (2 * (0x0fff / 5)), (3 * (0x0fff / 5)), (4 * (0x0fff / 5)), (0x0fff)};
    static bool readAllValues = false;

    if(levelNumber >= FAN_LEVEL_COUNT){
        return false;
    }

    #if CFG_FACTORY_PARTITION_DISABLE
        readAllValues = true;
    #endif

    if(readAllValues == true){
        *pwmValue =  sFanPwmValue[levelNumber];
    
        return true;
    }

    nvs_handle_t nvsHandle;

    esp_err_t res = nvs_open_from_partition(PARTITION_NAME, NVS_STORAGE_NAMESPACE, NVS_READONLY, &nvsHandle);
    if(res != ESP_OK){
        return false;
    }

    for(uint16_t idx = 0; idx < FAN_LEVEL_COUNT; ++idx){
        res = nvs_get_u32 (nvsHandle, sFanPwmKeyName[idx], &sFanPwmValue[idx]);
        if(res != ESP_OK){
            nvs_close(nvsHandle);

            return false;
        }
        ESP_LOGI(TAG, "Fan pwm level %d = %d", (idx + 1), sFanPwmValue[idx]);
    }

    readAllValues = true;
    nvs_close(nvsHandle);

    *pwmValue =  sFanPwmValue[levelNumber];

    return true;
}

bool FactorySettingsGetScheduler(Scheduler_t* scheduler)
{
    #if CFG_FACTORY_PARTITION_DISABLE
        memset(scheduler, 0, sizeof(Scheduler_t));
        return true;
    #endif

    nvs_handle_t nvsHandle;
    esp_err_t err = nvs_open_from_partition(PARTITION_NAME, NVS_STORAGE_NAMESPACE, NVS_READONLY, &nvsHandle);
    if(err != ESP_OK){
        return false;
    }

    bool res = true;
    size_t lenght = sizeof(Scheduler_t);

    err = nvs_get_blob(nvsHandle, SCHEDULER_KAY_NAME, scheduler, &lenght);
    if(err != ESP_OK){
        res = false;
    }else{
        SchedulerPrintf(scheduler);
        ESP_LOGI(TAG, "lenght %d", lenght);
    }

    nvs_close(nvsHandle);

    return res;
}

bool FactorySettingsDriverInit(void)
{
    ESP_LOGI(TAG, "nvs_flash_init_partition");
    #if CFG_FACTORY_PARTITION_DISABLE
        ESP_LOGI(TAG, "factory partition disable");
        return true;
    #endif
    
    ESP_ERROR_CHECK(nvs_flash_init_partition(PARTITION_NAME));

    return true;
}

bool FactorySettingsGetServiceParam(FactorySettingServiceParam_t serviceParam, uint32_t* serviceValue)
{
    static bool readAllValues = false;

    if(serviceParam >= FACTORY_SETTING_SERVICE_COUNT){
        return false;
    }

    #if CFG_FACTORY_PARTITION_DISABLE
        readAllValues = true;
    #endif

    if(readAllValues == true){
        *serviceValue =  sServiceParameter[serviceParam];
    
        return true;
    }

    nvs_handle_t nvsHandle;
    esp_err_t res = nvs_open_from_partition(PARTITION_NAME, NVS_STORAGE_NAMESPACE, NVS_READONLY, &nvsHandle);
    if(res != ESP_OK){
        return false;
    }

    for(uint16_t idx = 0; idx < FACTORY_SETTING_SERVICE_COUNT; ++idx){
        res = nvs_get_u32(nvsHandle, sServiceSettingKeyName[idx], &sServiceParameter[idx]);
        if(res != ESP_OK){
            nvs_close(nvsHandle);

            return false;
        }
        ESP_LOGI(TAG, "Service settings %s = %d", sServiceSettingKeyName[idx], sServiceParameter[idx]);
    }

    readAllValues = true;
    nvs_close(nvsHandle);

    *serviceValue =  sServiceParameter[serviceParam];

    return true;
}

bool FactorySettingsUpdateServiceParam(FactorySettingServiceParam_t serviceParam, uint32_t* serviceValue)
{
    if(serviceParam >= FACTORY_SETTING_SERVICE_COUNT){
        return false;
    }

    #if CFG_FACTORY_PARTITION_DISABLE
        return true;
    #endif

    if(*serviceValue == sServiceParameter[serviceParam]){
        // want to save the same value
        return true;
    }

    nvs_handle_t nvsHandle;
    esp_err_t res = nvs_open_from_partition(PARTITION_NAME, NVS_STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);
    if(res != ESP_OK){
        return false;
    }

    res = nvs_set_u32(nvsHandle, sServiceSettingKeyName[serviceParam], *serviceValue);
    if(res != ESP_OK){
        nvs_close(nvsHandle);
        return false;
    }    
    nvs_close(nvsHandle);
    
    sServiceParameter[serviceParam] = *serviceValue;
    ESP_LOGI(TAG, "Service settings %s = %d", sServiceSettingKeyName[serviceParam], sServiceParameter[serviceParam]);

    return true;
}

char* FactorySettingsGetServicePassword(void)
{
    static char sServicePassword[SERVICE_PASSWORD_STRING_LEN + 1] = {};

    if(sServicePassword[0] != 0){
        return sServicePassword;
    }

    if(GetStringFromPartition(SERVICE_PASSWORD_KAY_NAME, sServicePassword, SERVICE_PASSWORD_STRING_LEN) == false){
        sServicePassword[0] = 0;
        return NULL;
    }

    ESP_LOGI(TAG, "Service password len %d", strlen(sDeviceNameStr));

    return sServicePassword;
}

char* FactorySettingsGetDiagnosticPassword(void)
{
    static char sDiagnosticPassword[DIAGNOSTIC_PASSWORD_STRING_LEN + 1] = {};

    if(sDiagnosticPassword[0] != 0){
        return sDiagnosticPassword;
    }

    if(GetStringFromPartition(DIAGNOSTIC_PASSWORD_KAY_NAME, sDiagnosticPassword, DIAGNOSTIC_PASSWORD_STRING_LEN) == false){
        sDiagnosticPassword[0] = 0;
        return NULL;
    }

    ESP_LOGI(TAG, "Service password len %d", strlen(sDiagnosticPassword));

    return sDiagnosticPassword;
}

char* FactorySettingsGetCloudHostName(void)
{
    static char sHostName[HOST_NAME_STRING_LEN + 1] = {};

    if(sHostName[0] != 0){
        return sHostName;
    }

    if(GetStringFromPartition(HOST_NAME_KAY_NAME, sHostName, HOST_NAME_STRING_LEN) == false){
        sHostName[0] = 0;
        return NULL;
    }

    ESP_LOGI(TAG, "host name len %d", strlen(sHostName));

    return sHostName;
}

char* FactorySettingsGetScopeIdName(void)
{
    static char sIdScopeName[ID_SCOPE_STRING_LEN + 1] = {};

    if(sIdScopeName[0] != 0){
        return sIdScopeName;
    }

    if(GetStringFromPartition(ID_SCOPE_KAY_NAME, sIdScopeName, ID_SCOPE_STRING_LEN) == false){
        sIdScopeName[0] = 0;
        return NULL;
    }

    ESP_LOGI(TAG, "id sope len %d", strlen(sIdScopeName));

    return sIdScopeName;
}

char* FactorySettingsGetClientCert(void)
{
    static char sClientCert[CLIENT_CERT_STRING_LEN + 1] = {};

    if(sClientCert[0] != 0){
        return sClientCert;
    }

    size_t clientCertSize = CLIENT_CERT_STRING_LEN;
    size_t interCertSize = CLIENT_CERT_STRING_LEN;
    size_t rootCertSize = CLIENT_CERT_STRING_LEN;

    if(GetBlobFromPartition(CLIENT_CERT_KAY_NAME, NULL, &clientCertSize) == false){
        sClientCert[0] = 0;
        return NULL;
    }

    if(GetBlobFromPartition(INTER_CERT_KAY_NAME, NULL, &interCertSize) == false){
        sClientCert[0] = 0;
        return NULL;
    }

    if(GetBlobFromPartition(ROOT_CERT_KAY_NAME, NULL, &rootCertSize) == false){
        sClientCert[0] = 0;
        return NULL;
    }

    ESP_LOGI(TAG, "client len %d", clientCertSize);
    ESP_LOGI(TAG, "inter len %d", interCertSize);
    ESP_LOGI(TAG, "root len %d", rootCertSize);

    if((clientCertSize + interCertSize + rootCertSize + 1) > CLIENT_CERT_STRING_LEN){
        ESP_LOGI(TAG, "client cerb buffer size too small %d > %d", (clientCertSize + interCertSize + rootCertSize + 1), CLIENT_CERT_STRING_LEN);

        sClientCert[0] = 0;
        return NULL;
    }

    uint16_t actualPos = 0;

    if(clientCertSize != 0){
        if(GetBlobFromPartition(CLIENT_CERT_KAY_NAME, &sClientCert[actualPos], &clientCertSize) == false){
            sClientCert[0] = 0;
            return NULL;
        }
        actualPos = clientCertSize;
    }

    if(interCertSize != 0){
        if(GetBlobFromPartition(INTER_CERT_KAY_NAME, &sClientCert[actualPos], &interCertSize) == false){
            sClientCert[0] = 0;
            return NULL;
        }
        actualPos += interCertSize;
    }

    if(rootCertSize != 0){
        if(GetBlobFromPartition(INTER_CERT_KAY_NAME, &sClientCert[actualPos], &rootCertSize) == false){
            sClientCert[0] = 0;
            return NULL;
        }
    }

    return sClientCert;
}

char* FactorySettingsGetClientKey(void)
{
    static char sClientKay[CLIENT_KEY_STRING_LEN + 1] = {};

    if(sClientKay[0] != 0){
        return sClientKay;
    }

    size_t clientKaySize = CLIENT_KEY_STRING_LEN;

    if(GetBlobFromPartition(CLIENT_KEY_KAY_NAME, sClientKay, &clientKaySize) == false){
        sClientKay[0] = 0;
        return NULL;
    }

    ESP_LOGI(TAG, "key len %d", clientKaySize);

    return sClientKay;
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool GetStringFromPartition(const char* kay, char* raedStr, size_t readLen)
{
    nvs_handle_t nvsHandle;

    esp_err_t res = nvs_open_from_partition(PARTITION_NAME, NVS_STORAGE_NAMESPACE, NVS_READONLY, &nvsHandle);
    if(res != ESP_OK){
        return false;
    }

    res = nvs_get_str(nvsHandle, kay, raedStr, &readLen);
    if(res != ESP_OK){
        nvs_close(nvsHandle);

        return false;
    }

    nvs_close(nvsHandle);
    return true;
}

static bool SetStringFromPartition(const char* kay, char* writeStr)
{
    nvs_handle_t nvsHandle;

    esp_err_t res = nvs_open_from_partition(PARTITION_NAME, NVS_STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);
    if(res != ESP_OK){
        return false;
    }

    res = nvs_set_str(nvsHandle, kay, writeStr);
    if(res != ESP_OK){
        nvs_close(nvsHandle);

        return false;
    }

    nvs_close(nvsHandle);
    return true;
}

static bool GetBlobFromPartition(const char* kay, char* raedStr, size_t* readLen)
{
    nvs_handle_t nvsHandle;

    esp_err_t res = nvs_open_from_partition(PARTITION_NAME, NVS_STORAGE_NAMESPACE, NVS_READONLY, &nvsHandle);
    if(res != ESP_OK){
        return false;
    }

    res = nvs_get_blob(nvsHandle, kay, raedStr, readLen);
    if(res != ESP_OK){
        nvs_close(nvsHandle);

        return false;
    }

    nvs_close(nvsHandle);
    return true;
}