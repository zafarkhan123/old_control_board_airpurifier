/**
 * @file webServer.c
 *
 * @brief Web Server source file
 *
 * @dir webServer
 * @brief Web Server folder
 *
 * @author matwal
 * @date 2021.08.25
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "webServer.h"
#include "esp_eth.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_tls_crypto.h"
#include "esp_vfs.h"
#include "nvs_flash.h"
#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <sys/param.h>

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#include "common/messageParserAndSerializer.h"
#include "common/messageType.h"

#include "scheduler/scheduler.h"
#include "wifi/wifi.h"

#include "mcuDriver/mcuDriver.h"
#include "timeDriver/timeDriver.h"
#include "rtcDriver/rtcDriver.h"
#include "timerDriver/timerDriver.h"

#include "device/alarmHandling.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
*****************************************************************************/

#define SOFT_AP_IP_ADDRESS_1 192
#define SOFT_AP_IP_ADDRESS_2 168
#define SOFT_AP_IP_ADDRESS_3 4
#define SOFT_AP_IP_ADDRESS_4 1

#define SOFT_AP_GW_ADDRESS_1 192
#define SOFT_AP_GW_ADDRESS_2 168
#define SOFT_AP_GW_ADDRESS_3 4
#define SOFT_AP_GW_ADDRESS_4 3

#define SOFT_AP_NM_ADDRESS_1 255
#define SOFT_AP_NM_ADDRESS_2 255
#define SOFT_AP_NM_ADDRESS_3 255
#define SOFT_AP_NM_ADDRESS_4 0

#define SERVER_PORT 3500

#define WEB_MOUNT_POINT "/www"
static const char *TAG = "web_server";
#define REST_CHECK(a, str, goto_tag, ...)                                      \
  do {                                                                         \
    if (!(a)) {                                                                \
      ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__);    \
      goto goto_tag;                                                           \
    }                                                                          \
  } while (0)

#define CHECK_FILE_EXTENSION(filename, ext)                                    \
  (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

#define AUTH_PASS ("{\"authenticate\":true}")
#define AUTH_FAIL ("{\"authenticate\":false}")

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/
typedef struct rest_server_context {
  char base_path[ESP_VFS_PATH_MAX + 1];
  char scratch[SCRATCH_BUFSIZE];
} __attribute__ ((packed)) rest_server_context_t;

static httpd_handle_t webServerInstance = NULL;
static SettingDevice_t sDeviceSetting;

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Get device info json
 *  @param req HTTP request data structure
 *  @return ESP_OK if succes
 */
static esp_err_t DeviceInfoHandler(httpd_req_t *req);

/** @brief Post device mode
 *  @param req HTTP request data structure
 *  @return ESP_OK if succes
 */
static esp_err_t DeviceModeHandler(httpd_req_t *req);

/** @brief Post device scheduler
 *  @param req HTTP request data structure
 *  @return ESP_OK if succes
 */
static esp_err_t DeviceSchedulerPostHandler(httpd_req_t *req);

/** @brief Post wifi setting
 *  @param req HTTP request data structure
 *  @return ESP_OK if succes
 */
static esp_err_t WifiSettingPostHandler(httpd_req_t *req);

/** @brief Get wifi setting
 *  @param req HTTP request data structure
 *  @return ESP_OK if succes
 */
static esp_err_t DeviceSchedulerGetHandler(httpd_req_t *req);

/** @brief Post time
 *  @param req HTTP request data structure
 *  @return ESP_OK if succes
 */
static esp_err_t DeviceTimePostHandler(httpd_req_t *req);

/** @brief Post reset counter
 *  @param req HTTP request data structure
 *  @return ESP_OK if succes
 */
static esp_err_t ResetCounterPostHandler(httpd_req_t *req);

/** @brief Post update
 *  @param req HTTP request data structure
 *  @return ESP_OK if succes
 */
static esp_err_t DeviceUplaodPostHandler(httpd_req_t *req);

/** @brief Get device diagnostic
 *  @param req HTTP request data structure
 *  @return ESP_OK if succes
 */
static esp_err_t DeviceDiagnosticGetHandler(httpd_req_t *req);

/** @brief Post device auth
 *  @param req HTTP request data structure
 *  @return ESP_OK if succes
 */
static esp_err_t DeviceAuthPostHandler(httpd_req_t *req);

/** @brief  Set HTTP response content type according to file extension
 *  @param req HTTP request data structure
 *  @param filepath Path to file
 *  @return ESP_OK if succes
 */
static esp_err_t SetContentTypeFromFile(httpd_req_t *req, const char *filepath);

/** @brief  Send HTTP response with the contents of the requested file
 *  @param req HTTP request data structure
 *  @return ESP_OK if succes
 */
static esp_err_t RestCommonGetHandler(httpd_req_t *req);

/** @brief Start Web Server
 *  @return ESP_OK if succes
 */
esp_err_t StartWebserver(const char *base_path);

/** @brief Initialize Access Point
 *  @return ESP_OK if succes
 */
static esp_err_t WifiInitAp(void);

/** @brief  Initialize filesystem
 *  @return ESP_OK if succes
 */
static esp_err_t InitFs(void);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool WebServerInit(void)
{
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  return true;
}

void WebServerStart(void) 
{
  ESP_LOGI(TAG, "init softAP");
  ESP_ERROR_CHECK(WifiInitAp());
  ESP_ERROR_CHECK(InitFs());

  ESP_ERROR_CHECK(StartWebserver(WEB_MOUNT_POINT));
}

void WebServerStop(void) 
{ 
  // Stop the httpd server
  httpd_stop(webServerInstance);
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static esp_err_t DeviceInfoHandler(httpd_req_t *req) {
  messageTypeDeviceInfo_t info = {};
  char jsonStr[MESSAGE_TYPE_MAX_DEVICE_INFO_JSON_LENGTH] = {};

  cJSON *jsonRoot = cJSON_CreateObject();

  SettingGet(&sDeviceSetting);
  MessageTypeCreateDeviceInfo(&info, &sDeviceSetting);
  MessageParserAndSerializerCreateDeviceInfoJson(jsonRoot, &info);

  if( ESP_OK != httpd_resp_set_type( req, "applicatio/json" ) ) {
	  ESP_LOGW( TAG, "Changing Content-Type in http header to application/json fails" );
  }

  if (cJSON_PrintPreallocated(jsonRoot, jsonStr, MESSAGE_TYPE_MAX_DEVICE_INFO_JSON_LENGTH, false) == false) {
    jsonStr[0] = 0;
    ESP_LOGE(TAG, "Device info Json size is too big");
  }

  cJSON_Delete(jsonRoot);
  httpd_resp_send(req, jsonStr, strlen(jsonStr));

  return ESP_OK;
}

static esp_err_t DeviceModeHandler(httpd_req_t *req) {
  char buf[MESSAGE_TYPE_MAX_DEVICE_MODE_JSON_LENGTH] = {};
  int ret;

  /* Read the data for the request */
  ret = httpd_req_recv(req, buf, MESSAGE_TYPE_MAX_DEVICE_MODE_JSON_LENGTH);
  if (ret <= 0) {
    return ESP_FAIL;
  }

  messageTypeDeviceMode_t mode = {};
  if( false == MessageParserAndSerializerParseDeviceModeJsonString(buf, &mode) ) {
	  ESP_LOGE( TAG, "fails parsing JSON" );
  }
  ESP_LOGI(TAG, "fan %u, switch %s, mode %s wifi %d", mode.fan, (mode.Switch ? "true" : "false"), (mode.automatical ? "AUTOMATICAL" : "MANUAL"), mode.wifiConnect);
  ESP_LOGI(TAG, "eco %s, lock %s", (mode.ecoOn ? "on" : "off"), (mode.lockOn ? "on" : "off"));

  SettingGet(&sDeviceSetting);
  MessageTypeCreateSettingDevice(&mode, &sDeviceSetting);
  SettingUpdateDeviceStatus(&sDeviceSetting);
  SettingUpdateDeviceMode(&sDeviceSetting);
  SettingUpdateTouchScreen(sDeviceSetting.restore.touchLock);
  // End response
  httpd_resp_send_chunk(req, NULL, 0);

  return ESP_OK;
}

static esp_err_t DeviceSchedulerPostHandler(httpd_req_t *req) {
  char buf[MESSAGE_TYPE_MAX_DEVICE_SCHEDULER_JSON_LENGTH] = {};
  int ret;

  /* Read the data for the request */
  ret = httpd_req_recv(req, buf, MESSAGE_TYPE_MAX_DEVICE_SCHEDULER_JSON_LENGTH);
  if (ret <= 0) {
    return ESP_FAIL;
  }
  
  messageTypeScheduler_t messageScheduler = {};

  if(MessageParserAndSerializerParseDeviceSchedulerJsonString(buf, &messageScheduler)){
    Scheduler_t scheduler = {};

    SchedulerGetAll(&scheduler);
    MessageTypeCreateScheduler(&messageScheduler, &scheduler);

    SchedulerSetAll(&scheduler);
    SchedulerSave();
    SchedulerPrintf(&scheduler); 
  }
  else{
    httpd_resp_set_status(req, HTTPD_400);
  }

  // End response
  httpd_resp_send_chunk(req, NULL, 0);

  return ESP_OK;
}

static esp_err_t WifiSettingPostHandler(httpd_req_t *req) {
  char buf[MESSAGE_TYPE_MAX_WIFI_SETTING_JSON_LENGTH] = {};
  int ret;

  /* Read the data for the request */
  ret = httpd_req_recv(req, buf, MESSAGE_TYPE_MAX_WIFI_SETTING_JSON_LENGTH);
  if (ret <= 0) {
    return ESP_FAIL;
  }
  
  wifiSetting_t wifiSetting = {};

  if(MessageParserAndSerializerParseWifiSettingJsonString(buf, &wifiSetting)){
    ESP_LOGI(TAG, "Set new wifi ssid %s", wifiSetting.ssid);
    wifiSetting.isSet = true;
    WifiSettingSave(&wifiSetting);

    SettingGet(&sDeviceSetting);
    sDeviceSetting.tryConnectToNewAp = true;
    sDeviceSetting.isConnectNewAp = false;
    SettingSet(&sDeviceSetting);

    WifiReinit();
  }
  else{
    httpd_resp_set_status(req, HTTPD_400);
  }

  // End response
  httpd_resp_send_chunk(req, NULL, 0);

  return ESP_OK;
}

static esp_err_t DeviceUplaodPostHandler(httpd_req_t *req) {
  esp_err_t err = OtaUploadByWebserver(req);
  if(err != ESP_OK){
    ESP_LOGW(TAG, "something went wrong");
    httpd_resp_set_status(req, HTTPD_400);
  }

  // End response
  httpd_resp_send_chunk(req, NULL, 0);
  vTaskDelay(1000U);

  if(err == ESP_OK){
    ESP_LOGI(TAG, "time for restart to complete the update");
    McuDriverDeviceSafeRestart();
  }

  return ESP_OK;
}

static esp_err_t DeviceTimePostHandler(httpd_req_t *req) {
  char buf[MESSAGE_TYPE_MAX_DEVICE_TIME_JSON_LENGTH] = {};
  int ret;

  /* Read the data for the request */
  ret = httpd_req_recv(req, buf, MESSAGE_TYPE_MAX_DEVICE_TIME_JSON_LENGTH);
  if (ret <= 0) {
    return ESP_FAIL;
  }
  
  struct tm newDataTime = {};
  float newOffset = 0;
  if(MessageParserAndSerializerParseDeviceTimeHttpClientJsonString(buf, &newDataTime, &newOffset) == true){
    ESP_LOGI(TAG, "old time %s", TimeDriverGetLocalTimeStr());

    TimeDriverSetEspTime(&newDataTime);
    RtcDriverSetDateTime(&newDataTime);

    ESP_LOGI(TAG, "new set time %s", TimeDriverGetLocalTimeStr());

    float actualOffset = LocationGetUtcOffset();
    if(actualOffset != newOffset){
      ESP_LOGI(TAG, "utc offset change %.1f", newOffset);
      LocationSetUtcOffset(newOffset);
      LocationSave();
    }

    SettingSave();
  }
  else{
    httpd_resp_set_status(req, HTTPD_400);
  }

  // End response
  httpd_resp_send_chunk(req, NULL, 0);

  return ESP_OK;
}

static esp_err_t ResetCounterPostHandler(httpd_req_t *req) {
  char buf[MESSAGE_TYPE_MAX_CLEAR_COUNTER_JSON_LENGTH] = {};
  int ret;

  /* Read the data for the request */
  ret = httpd_req_recv(req, buf, MESSAGE_TYPE_MAX_DEVICE_TIME_JSON_LENGTH);
  if (ret <= 0) {
    return ESP_FAIL;
  }

  messageTypeClearCounter_t messageTypeClearCounter = {};
  if(MessageParserAndSerializerParseDeviceCounterHttpClientJsonString(buf, &messageTypeClearCounter) == true){
    if(messageTypeClearCounter.hepaCounter == true){
      ESP_LOGI(TAG, "hepa counter clear");
      SettingGet(&sDeviceSetting);

      TimerDriverClearCounter(TIMER_NAME_HEPA);

      TimerDriverUpdateTimerSetting(&sDeviceSetting);
      AlarmHandlingTimersWornOutCheck(&sDeviceSetting);

      SettingSet(&sDeviceSetting);
      SettingSave();
    }

    if(messageTypeClearCounter.uvLamp1Counter == true){
      ESP_LOGI(TAG, "uv lamp 1 counter clear");
      SettingGet(&sDeviceSetting);

      TimerDriverClearCounter(TIMER_NAME_UV_LAMP_1);

      TimerDriverUpdateTimerSetting(&sDeviceSetting);
      AlarmHandlingTimersWornOutCheck(&sDeviceSetting);

      SettingSet(&sDeviceSetting);
      SettingSave();
    }

    if(messageTypeClearCounter.uvLamp2Counter == true){
      ESP_LOGI(TAG, "uv lamp 2 counter clear");
      SettingGet(&sDeviceSetting);

      TimerDriverClearCounter(TIMER_NAME_UV_LAMP_2);

      TimerDriverUpdateTimerSetting(&sDeviceSetting);
      AlarmHandlingTimersWornOutCheck(&sDeviceSetting);

      SettingSet(&sDeviceSetting);
      SettingSave();
    }
  }
  else{
    httpd_resp_set_status(req, HTTPD_400);
  }

  // End response
  httpd_resp_send_chunk(req, NULL, 0);

  return ESP_OK;
}

static esp_err_t DeviceSchedulerGetHandler(httpd_req_t *req) {
  Scheduler_t scheduler = {};
  messageTypeScheduler_t messageScheduler = {};
  char jsonStr[MESSAGE_TYPE_MAX_DEVICE_SCHEDULER_JSON_LENGTH] = {};
  
  SchedulerGetAll(&scheduler);
  MessageTypeCreateMessageTypeScheduler(&messageScheduler, &scheduler);

  if( ESP_OK != httpd_resp_set_type( req, "applicatio/json" ) ) {
	  ESP_LOGW( TAG, "Changing Content-Type in http header to application/json fails" );
  }

  cJSON *jsonRoot = cJSON_CreateObject();
  MessageParserAndSerializerCreateSchedulerJson(jsonRoot, &messageScheduler);

  if (cJSON_PrintPreallocated(jsonRoot, jsonStr, MESSAGE_TYPE_MAX_DEVICE_SCHEDULER_JSON_LENGTH, false) == false) {
    jsonStr[0] = 0;
    ESP_LOGE(TAG, "Scheduler Json size is too big");
  }

  cJSON_Delete(jsonRoot);
  httpd_resp_send(req, jsonStr, strlen(jsonStr));

  return ESP_OK;
}

static esp_err_t DeviceDiagnosticGetHandler(httpd_req_t *req) {
  messageTypeDiagnostic_t diagn = {};
  char jsonStr[MESSAGE_TYPE_MAX_DEVICE_DIAGNOSTIC_JSON_LENGTH] = {};

  cJSON *jsonRoot = cJSON_CreateObject();

  SettingGet(&sDeviceSetting);
  MessageTypeCreateDeviceDiagnostic(&diagn, &sDeviceSetting);
  MessageParserAndSerializerCreateDeviceDiagnosticJson(jsonRoot, &diagn);

  if( ESP_OK != httpd_resp_set_type( req, "applicatio/json" ) ) {
	  ESP_LOGW( TAG, "Changing Content-Type in http header to application/json fails" );
  }

  if (cJSON_PrintPreallocated(jsonRoot, jsonStr, MESSAGE_TYPE_MAX_DEVICE_DIAGNOSTIC_JSON_LENGTH, false) == false) {
    jsonStr[0] = 0;
    ESP_LOGE(TAG, "Device info Json size is too big");
  }

  cJSON_Delete(jsonRoot);
  httpd_resp_send(req, jsonStr, strlen(jsonStr));

  return ESP_OK;
}

static esp_err_t DeviceAuthPostHandler(httpd_req_t *req) {
  char buf[MESSAGE_TYPE_MAX_DEVICE_AUTH_JSON_LENGTH] = {};
  int ret;

  /* Read the data for the request */
  ret = httpd_req_recv(req, buf, MESSAGE_TYPE_MAX_DEVICE_AUTH_JSON_LENGTH);
  if (ret <= 0) {
    return ESP_FAIL;
  }

  messageTypeDeviceAuthType_t auth = MESSAGE_TYPE_AUTH_TYPE_FAIL;
  
  if(MessageParserAndSerializerParseDeviceAuthJsonString(buf, &auth) == false) {
	  ESP_LOGE(TAG, "fails parsing JSON");
  }

  ESP_LOGI(TAG, "auth %d", auth);

  if((auth == MESSAGE_TYPE_AUTH_TYPE_SERVICE) || (auth == MESSAGE_TYPE_AUTH_TYPE_DIAGNOSTIC)){
    // End response
    httpd_resp_send(req, AUTH_PASS, strlen(AUTH_PASS));
  }
  else{
    httpd_resp_send(req, AUTH_FAIL, strlen(AUTH_FAIL));
  }
  
  return ESP_OK;
}

 esp_err_t StartWebserver(const char *base_path) {

  REST_CHECK(base_path, "wrong base path", err);
  rest_server_context_t *rest_context =
      calloc(1, sizeof(rest_server_context_t));
  REST_CHECK(rest_context, "No memory for rest context", err);
  strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 12;
  config.stack_size = (10U * 1024U); // Increase  stack size

  config.lru_purge_enable = true;
  config.uri_match_fn = httpd_uri_match_wildcard;

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  REST_CHECK(httpd_start(&webServerInstance, &config) == ESP_OK, "Start server failed", err_start);
  ESP_LOGI(TAG, "Registering URI handlers");

  /* URI handler for getting diagnostic json */
  httpd_uri_t deviceDiagnosticGet = 
  {
    .uri = "/deviceDiag",
    .method = HTTP_GET,
    .handler = DeviceDiagnosticGetHandler,
    .user_ctx = rest_context
  };
  httpd_register_uri_handler(webServerInstance, &deviceDiagnosticGet);

  /* URI handler for post device auth json */
  httpd_uri_t deviceAuthPost = 
  {
    .uri = "/deviceAuth",
    .method = HTTP_POST,
    .handler = DeviceAuthPostHandler,
    .user_ctx = rest_context
  };
  httpd_register_uri_handler(webServerInstance, &deviceAuthPost);

  /* URI handler for getting device info json */
  httpd_uri_t deviceInfo = 
  {
    .uri = "/deviceInfo",
    .method = HTTP_GET,
    .handler = DeviceInfoHandler,
    .user_ctx = rest_context
  };
  httpd_register_uri_handler(webServerInstance, &deviceInfo);

  /* URI handler for getting scheduler json */
  httpd_uri_t deviceScheduleGet = 
  {
    .uri = "/deviceSchedule",
    .method = HTTP_GET,
    .handler = DeviceSchedulerGetHandler,
    .user_ctx = rest_context
  };
  httpd_register_uri_handler(webServerInstance, &deviceScheduleGet);

  /* URI handler for post device mode */
  httpd_uri_t deviceMode = 
  {
    .uri = "/deviceMode",
    .method = HTTP_POST,
    .handler = DeviceModeHandler,
    .user_ctx = rest_context
  };
  httpd_register_uri_handler(webServerInstance, &deviceMode);

  /* URI handler for getting web server files */
  httpd_uri_t commonGetUri = 
  {
    .uri = "/*",
    .method = HTTP_GET,
    .handler = RestCommonGetHandler,
    .user_ctx = rest_context
  };
  httpd_register_uri_handler(webServerInstance, &commonGetUri);

  /* URI handler for post scheduler */
  httpd_uri_t deviceSchedulePost = 
  {
    .uri = "/deviceSchedule",
    .method = HTTP_POST,
    .handler = DeviceSchedulerPostHandler,
    .user_ctx = rest_context
  };
  httpd_register_uri_handler(webServerInstance, &deviceSchedulePost);

  /* URI handler for post wifiSetting */
  httpd_uri_t deviceWifiSettingPost = 
  {
    .uri = "/wifiSetting",
    .method = HTTP_POST,
    .handler = WifiSettingPostHandler,
    .user_ctx = rest_context
  };
  httpd_register_uri_handler(webServerInstance, &deviceWifiSettingPost);

  /* URI handler for post deviceUpload */
  httpd_uri_t deviceDeviceUplaodPost = 
  {
    .uri = "/upload",
    .method = HTTP_POST,
    .handler = DeviceUplaodPostHandler,
    .user_ctx = rest_context
  };
  httpd_register_uri_handler(webServerInstance, &deviceDeviceUplaodPost);

  /* URI handler for post time */
  httpd_uri_t deviceDeviceTimePost = 
  {
    .uri = "/time",
    .method = HTTP_POST,
    .handler = DeviceTimePostHandler,
    .user_ctx = rest_context
  };
  httpd_register_uri_handler(webServerInstance, &deviceDeviceTimePost);

  /* URI handler for post resetcounter */
  httpd_uri_t deviceResetCounterPost = 
  {
    .uri = "/resetcounter",
    .method = HTTP_POST,
    .handler = ResetCounterPostHandler,
    .user_ctx = rest_context
  };
  httpd_register_uri_handler(webServerInstance, &deviceResetCounterPost);

  return ESP_OK;
err_start:
  free(rest_context);
err:
  return ESP_FAIL;
}

static esp_err_t WifiInitAp(void) {
  ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));

  tcpip_adapter_ip_info_t ipAddressInfo = {};

  IP4_ADDR(&ipAddressInfo.ip, SOFT_AP_IP_ADDRESS_1, SOFT_AP_IP_ADDRESS_2, SOFT_AP_IP_ADDRESS_3, SOFT_AP_IP_ADDRESS_4);
  IP4_ADDR(&ipAddressInfo.gw, SOFT_AP_GW_ADDRESS_1, SOFT_AP_GW_ADDRESS_2, SOFT_AP_GW_ADDRESS_3, SOFT_AP_GW_ADDRESS_4);
  IP4_ADDR(&ipAddressInfo.netmask, SOFT_AP_NM_ADDRESS_1, SOFT_AP_NM_ADDRESS_2, SOFT_AP_NM_ADDRESS_3, SOFT_AP_NM_ADDRESS_4);

  ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ipAddressInfo));
  ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));

  return ESP_OK;
}

static esp_err_t InitFs(void) {
  esp_vfs_spiffs_conf_t conf = 
  {
    .base_path = WEB_MOUNT_POINT,
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = false
  };
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    } else {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return ESP_FAIL;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)",
             esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
  return ESP_OK;
}

static esp_err_t SetContentTypeFromFile(httpd_req_t *req, const char *filepath) {
  const char *type = "text/plain";
  if (CHECK_FILE_EXTENSION(filepath, ".html")) {
    type = "text/html";
  } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
    type = "application/javascript";
  } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
    type = "text/css";
  } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
    type = "image/png";
  } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
    type = "image/x-icon";
  } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
    type = "image/svg+xml";
  }
  return httpd_resp_set_type(req, type);
}

static esp_err_t RestCommonGetHandler(httpd_req_t *req) {
  char filepath[FILE_PATH_MAX];

  rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
  strlcpy(filepath, rest_context->base_path, sizeof(filepath));
  if (req->uri[strlen(req->uri) - 1] == '/') {
    strlcat(filepath, "/index.html", sizeof(filepath));
  } else {
    strlcat(filepath, req->uri, sizeof(filepath));
  }
  int fd = open(filepath, O_RDONLY, 0);
  if (fd == -1) {
    ESP_LOGE(TAG, "Failed to open file : %s", filepath);
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Failed to read existing file");
    return ESP_FAIL;
  }

  SetContentTypeFromFile(req, filepath);

  char *chunk = rest_context->scratch;
  ssize_t read_bytes;
  do {
    /* Read file in chunks into the scratch buffer */
    read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
    if (read_bytes == -1) {
      ESP_LOGE(TAG, "Failed to read file : %s", filepath);
    } else if (read_bytes > 0) {
      /* Send the buffer contents as HTTP response chunk */
      if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
        close(fd);
        ESP_LOGE(TAG, "File sending failed!");
        /* Abort sending file */
        httpd_resp_sendstr_chunk(req, NULL);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "Failed to send file");
        return ESP_FAIL;
      }
    }
  } while (read_bytes > 0);
  /* Close file after sending complete */
  close(fd);
  ESP_LOGI(TAG, "File sending complete");
  /* Respond with an empty chunk to signal HTTP response completion */
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}
