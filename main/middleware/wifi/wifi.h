/**
 * @file wifi.h
 *
 * @brief Wifi middleware header file
 *
 * @author matfio
 * @date 2021.11.16
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "esp_wpa2.h"
#include "esp_wifi_types.h"

#include "config.h"
#include "setting.h"

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

#define WIFI_SSID_STRING_NAME_LEN (CFG_WIFI_AP_SSID_STRING_LEN + 1U) // +1 adding a place for null
#define WIFI_PASSWORD_STRING_NAME_LEN (63U)

#define WIFI_RADIUS_SERVER_ADDRESS_LEN (63U)
#define WIFI_PEAP_USEN_LEN (128U)
#define WIFI_PEAP_PASSWORD_LEN (128U)

#define WIFI_WPA2_CA_PEM_FILE_NAME ("wpa2_ca_pem_blob")
#define WIFI_WPA2_CLIENT_CRT_FILE_NAME ("wpa2_client_crt")
#define WIFI_WPA2_CLIENT_KEY_FILE_NAME ("wpa2_client_key")

/*****************************************************************************
                       PUBLIC STRUCTS
*****************************************************************************/
typedef enum{
    WIFI_EAP_METHOD_NONE = 0,
    WIFI_EAP_METHOD_TLS,
    WIFI_EAP_METHOD_PEAP,
    WIFI_EAP_METHOD_TTLS,
    WIFI_EAP_METHOD_COUNT,
}wifiEapMethod_t;

typedef struct
{
    uint8_t isSet : 1;
    char ssid[WIFI_SSID_STRING_NAME_LEN];
    char password[WIFI_PASSWORD_STRING_NAME_LEN];
    wifiEapMethod_t eapMethod;
    uint8_t validateServer : 1;
    char radiusServerAddress[WIFI_RADIUS_SERVER_ADDRESS_LEN];
    char wpa2PeapEapUser[WIFI_PEAP_USEN_LEN];
    char wpa2PeapPassword[WIFI_PEAP_PASSWORD_LEN];
    esp_eap_ttls_phase2_types phase2Method;
} __attribute__ ((packed))  wifiSetting_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Initialize wifi connection
 *  @return return true if success
 */
bool WifiInit(void);

/** @brief Load wifi setting (ssid, password) from nvs
 *  @return return true if success
 */
bool WifiSettingInit(void);

/** @brief Stop wifi
 *  @return return true if success
 */
bool WifiStop(void);

/** @brief Start wifi
 *  @return return true if success
 */
bool WifiStart(void);

/** @brief Connect wifi sta
 *  @return return true if success
 */
bool WifiStaConnect(void);

/** @brief Save wifi setting (ssid, password) to nvs
 *  @param setting pointer to wifiSetting_t
 *  @return return true if success
 */
bool WifiSettingSave(wifiSetting_t* setting);

/** @brief Get wifi setting (ssid, password)
 *  @param setting pointer to wifiSetting_t
 *  @return return true if success
 */
bool WifiSettingGet(wifiSetting_t* setting);

/** @brief Get wifi sta status
 *  @return return status
 */
SettingWifiStatus_t WifiGetStaStatus(void);

/** @brief Reinitilize wifi
 *  @return return true success
 */
bool WifiReinit(void);

/** @brief RF is on
 *  @return return true if yes
 */
bool WifiRfEmit(void);

/** @brief Get actual wifi mode
 *  @return mode
 */
wifi_mode_t WifiModeGet(void);

/** @brief Get time when new ssid and password is set
 *  @return set time
 */
int64_t WifiGetNewApConnectionTime(void);