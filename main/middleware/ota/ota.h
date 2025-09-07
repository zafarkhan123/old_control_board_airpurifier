/**
 * @file ota.h
 *
 * @brief Over The Air Updates (OTA) middleware header file
 *
 * @author matfio
 * @date 2021.11.26
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <esp_http_server.h>

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

#define OTA_NEW_FIRMWARE_URL_STRING_LEN (256U)
#define OTA_NEW_VERSION_STRING_LEN (32U)

typedef enum{
    OTA_ERROR_DOWNLOAD_TO_LONG_INCOMPLETE_FILE = -10,
    OTA_ERROR_DOWNLOAD_TO_LONG = -9,
    OTA_ERROR_PACKAGE_CHECK_VALUE_INCORRECT = -8,
    OTA_ERROR_INVALID_IMAGE = -7,
    OTA_ERROR_INCORRECT_DATA_IN_IMAGE = -6,
    OTA_ERROR_READ_HTTP = -5,
    OTA_ERROR_PARTITION_PROBLEM = -4,
    OTA_ERROR_INCORRECT_SIZE = -3,
    OTA_ERROR_INCORRECT_ADDRESS = -2,
    OTA_ERROR_UNKNOWN = -1,
    OTA_NOTHING_TO_DO = 0,
    OTA_DOWNLOADING = 1,
    OTA_DOWNLOADED,
}OtaStatus_t;

/*****************************************************************************
                       PUBLIC STRUCT
*****************************************************************************/

typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t subMinor;
} __attribute__ ((packed)) OtaFirmwareVersion_t;

typedef struct {
    uint8_t isAvailable : 1;
    OtaFirmwareVersion_t version;
    char firmwareUrl[OTA_NEW_FIRMWARE_URL_STRING_LEN];
    uint32_t checksum;
} __attribute__ ((packed)) Ota_t;

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

/** @brief Get actual running firmware version
 *  @param version [out] pointer to OtaFirmwareVersion_t
 *  @return return true if success
 */
bool OtaGetFirmwareVersion(OtaFirmwareVersion_t *version);

/** @brief Create update task and run if it is necessary
 *  @param updateCandidate [in] pointer to Ota_t struct read from get
 *  @return return true if success
 */
void OtaCreateTask(Ota_t* updateCandidate);

/** @brief Create update task and run if it is necessary
 *  @param updateCandidate [in] pointer to Ota_t struct read from get
 *  @return return true if success
 */
bool OtaIsUpdateNeeded(Ota_t* updateCandidate);

/** @brief Check if actual running firmware is not verified yet
 *  @return return true if veryfication is need
 */
bool OtaIsVeryficationNeed(void);

/** @brief Get update ota status
 *  @return return OtaStatus_t 
 */
OtaStatus_t OtaGetStatus(void);

/** @brief Indicate that the running app is working well
 */
void  OtaMarkValid(void);

/** @brief Indicate that the running app is not work well and need rollback to the previous version
 */
void  OtaMarkInvalid(void);

/** @brief Upload send by post bin file and save in free partition
 *  @param req HTTP request data structure
 *  @return return status 
 */
esp_err_t OtaUploadByWebserver(httpd_req_t* req);