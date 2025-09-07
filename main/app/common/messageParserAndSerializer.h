/**
 * @file messageParserAndSerializer.h
 *
 * @brief messageParserAndSerializer header file
 *
 * @author matfio
 * @date 2021.09.01
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include "cJSON.h"
#include <time.h>

#include "messageType.h"

#include "scheduler/scheduler.h"
#include "location/location.h"
#include "wifi/wifi.h"
#include "ota/ota.h"

/*****************************************************************************
                     PUBLIC STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

typedef enum{
    MESSAGE_TYPE_UNKNOWN = -1,
    MESSAGE_TYPE_DEVICE_INFO = 0,
    MESSAGE_TYPE_DEVICE_STATUS,
    MESSAGE_TYPE_DEVICE_LOCATION,
    MESSAGE_TYPE_DEVICE_SCHEDULE,
    MESSAGE_TYPE_DEVICE_MODE,
    MESSAGE_TYPE_DEVICE_SERVICE,
    MESSAGE_TYPE_DEVICE_UPDATE,
    MESSAGE_TYPE_DEVICE_COUNT
}MessageType_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Parse Json and get message type
 *  @param deviceTypeBody [in] pointer to json string
 *  @return return MessageType_t
 */
MessageType_t MessageParserAndSerializerGetMessageType(const char * const deviceTypeBody);

/** @brief Create Json from messageTypeDeviceInfo_t type
 *  @param root [in] pointer to cJSON structure
 *  @param deviceInfo [in] pointer to messageTypeDeviceInfo_t structure
 *  @return return true if success
 */
bool MessageParserAndSerializerCreateDeviceInfoJson(cJSON *root, const messageTypeDeviceInfo_t* deviceInfo);

/** @brief Parse Json and write result to messageTypeDeviceMode_t type
 *  @param deviceModeBody [in] pointer to json string
 *  @param deviceMode [out] pointer to  messageTypeDeviceMode_t result
 *  @return return true if success
 */
bool MessageParserAndSerializerParseDeviceModeJsonString(const char * const deviceModeBody, messageTypeDeviceMode_t* deviceMode);

/** @brief Parse Json and write result to Scheduler_t type
 *  @param deviceModeBody [in] pointer to json string
 *  @param deviceMode [out] pointer to  Scheduler_t result
 *  @return return true if success
 */
bool MessageParserAndSerializerParseDeviceSchedulerJsonString(const char * const deviceSchedulerBody, messageTypeScheduler_t* scheduler);

/** @brief Create Json from messageTypeScheduler_t type
 *  @param root [in] pointer to cJSON structure
 *  @param deviceInfo [in] pointer to messageTypeScheduler_t structure
 *  @return return true if success
 */
bool MessageParserAndSerializerCreateSchedulerJson(cJSON *root, const messageTypeScheduler_t* scheduler);

/** @brief Parse Json and write result to wifiSetting_t type
 *  @param wifiSettingBody [in] pointer to json string
 *  @param wifiSetting [out] pointer to  wifiSetting_t result
 *  @return return true if success
 */
bool MessageParserAndSerializerParseWifiSettingJsonString(const char * const wifiSettingBody, wifiSetting_t* wifiSetting);

/** @brief Create Json from messageTypeDeviceInfoHttpClient_t type
 *  @param root [in] pointer to cJSON structure
 *  @param deviceInfo [in] pointer to messageTypeDeviceInfoHttpClient_t structure
 *  @return return true if success
 */
bool MessageParserAndSerializerCreateDeviceInfoHttpClientJson(cJSON *root, const messageTypeDeviceInfoHttpClient_t* deviceInfo);

/** @brief Create Json from messageTypeDeviceStatusHttpClient_t type
 *  @param root [in] pointer to cJSON structure
 *  @param deviceStatus [in] pointer to messageTypeDeviceStatusHttpClient_t structure
 *  @return return true if success
 */
bool MessageParserAndSerializerCreateDeviceStatusHttpClientJson(cJSON *root, const messageTypeDeviceStatusHttpClient_t* deviceStatus);

/** @brief Create Json from Location_t type
 *  @param root [in] pointer to cJSON structure
 *  @param deviceLocation [in] pointer to Location_t structure
 *  @return return true if success
 */
bool MessageParserAndSerializerCreateDeviceLocationHttpClientJson(cJSON *root, const Location_t* deviceLocation);

/** @brief Parse Json and write result to Location_t type
 *  @param deviceLocationBody [in] pointer to json string
 *  @param location [out] pointer to  Location_t result
 *  @return return true if success
 */
bool MessageParserAndSerializerParseDeviceLocationHttpClientJsonString(const char * const deviceLocationBody, Location_t* location);

/** @brief Parse Json and write result to messageTypeDeviceModeHttpClient_t type
 *  @param deviceModeBody [in] pointer to json string
 *  @param deviceMode [out] pointer to  messageTypeDeviceModeHttpClient_t result
 *  @return return true if success
 */
bool MessageParserAndSerializerParseDeviceModeHttpClientJsonString(const char * const deviceModeBody, messageTypeDeviceModeHttpClient_t* deviceMode);

/** @brief Parse Json and write result to messageTypeDeviceServiceHttpClient_t type
 *  @param deviceServiceBody [in] pointer to json string
 *  @param deviceService [out] pointer to  messageTypeDeviceServiceHttpClient_t result
 *  @return return true if success
 */
bool MessageParserAndSerializerParseDeviceServiceHttpClientJsonString(const char * const deviceServiceBody, messageTypeDeviceServiceHttpClient_t* deviceService);

/** @brief Parse Json and write result to Ota_t type
 *  @param deviceUpdateBody [in] pointer to json string
 *  @param ota [out] pointer to  Ota_t result
 *  @return return true if success
 */
bool MessageParserAndSerializerParseDeviceUpdateHttpClientJsonString(const char * const deviceUpdateBody, Ota_t* ota);

/** @brief Parse Json and write result to tm struct type
 *  @param deviceTimeBody [in] pointer to json string
 *  @param time [out] pointer to  struct tm
 *  @param offset [out] time offset
 *  @return return true if success
 */
bool MessageParserAndSerializerParseDeviceTimeHttpClientJsonString(const char * const deviceTimeBody, struct tm *time, float *offset);

/** @brief Parse Json and write result to tm struct type
 *  @param deviceCounterBody [in] pointer to json string
 *  @param counter [out] pointer to struct messageTypeClearCounter_t
 *  @return return true if success
 */
bool MessageParserAndSerializerParseDeviceCounterHttpClientJsonString(const char * const deviceCounterBody, messageTypeClearCounter_t *counter);

/** @brief Create Json from messageTypeDiagnostic_t type
 *  @param root [in] pointer to cJSON structure
 *  @param deviceDiag [in] pointer to messageTypeDiagnostic_t structure
 *  @return return true if success
 */
bool MessageParserAndSerializerCreateDeviceDiagnosticJson(cJSON *root, const messageTypeDiagnostic_t* deviceDiag);

/** @brief Parse Json and write result
 *  @param deviceAuthBody [in] pointer to json string
 *  @param counter [out] pointer to enum messageTypeDeviceAuthType_t
 *  @return return true if success
 */
bool MessageParserAndSerializerParseDeviceAuthJsonString(const char * const deviceAuthBody, messageTypeDeviceAuthType_t* deviceAuth);