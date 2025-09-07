/**
 * @file rtcDriver.c
 *
 * @brief RTC driver source file
 *
 * @dir rtcDriver
 * @brief Rtc driver folder
 *
 * @author matfio
 * @date 2021.11.05
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "config.h"
#include "setting.h"

#include "rtcDriver.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#include "adcDriver/adcDriver.h"

#include "cloud/iotHubClient.h"

#include "driver/i2c.h"
#include "hal/i2c_types.h"

#include <sys/time.h>
#include "esp_timer.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define I2C_PORT_NUMBER (CFG_I2C_PORT_NUMBER)

#define TIMER_PERIOD_MS (60U * 1000U) // 1 minute
#define ADC_MEAN_SAMPLES_NUMBER (16U)

#define MUTEX_TIMEOUT_MS (1U * 1000U)

#define PCF85063A_SLAVE_ADDRESS (0x51)

#define PCF85063A_WTITE_TIMEOUT_MS (1000U)
#define PCF85063A_READ_TIMEOUT_MS (1000U)

#define RTC_IS_CORRECT_YEAR_SET (2022U)
#define DEFAULT_RTC_CORRECTION_TIME_SEC (2U)

#define RTC_CRISTAL_FREQUENCY (32768.0f)
#define RTC_CRISTAL_PPM (20.0f)

/*****************************************************************************
                     PRIVATE ENUMS
*****************************************************************************/

typedef enum 
{
    PCF85063A_CONTROL_1             = 0x00,
    PCF85063A_CONTROL_2             = 0x01,
    PCF85063A_OFFSET                = 0x02,
    PCF85063A_RAM                   = 0x03,
    PCF85063A_SECONDS               = 0x04,
    PCF85063A_MINUTES               = 0x05,
    PCF85063A_HOURS                 = 0x06,
    PCF85063A_DAYS                  = 0x07,
    PCF85063A_WEEKDAYS              = 0x08,
    PCF85063A_MONTHS                = 0x09,
    PCF85063A_YEARS                 = 0x0A,
    PCF85063A_SECOND_ALARM          = 0x0B,
    PCF85063A_MINUTE_ALARM          = 0x0C,
    PCF85063A_HOUR_ALARM            = 0x0D,
    PCF85063A_DAY_ALARM             = 0x0E,
    PCF85063A_WEEKDAY_ALARM         = 0x0F,
    PCF85063A_TIMER_VALUE           = 0x10,
    PCF85063A_TIMER_MODE            = 0x11,
}RtcPcf85063aRegAddr_t;

/*****************************************************************************
                     PRIVATE STRUCTS
*****************************************************************************/

typedef struct
{
    uint8_t capSet          : 1;
    uint8_t mode12_24       : 1;
    uint8_t cie             : 1;
    uint8_t                 : 1;
    uint8_t sr              : 1;
    uint8_t stop            : 1;
    uint8_t                 : 1;
    uint8_t extTest         : 1;
} __attribute__ ((packed)) RtcPcf85063aControl_1_t;

typedef struct
{
    uint8_t cof             : 3;
    uint8_t tf              : 1;
    uint8_t hmi             : 1;
    uint8_t mi              : 1;
    uint8_t af              : 1;
    uint8_t aie             : 1;
} __attribute__ ((packed)) RtcPcf85063aControl_2_t;

typedef struct
{
    uint8_t offset          : 7;
    uint8_t mode            : 1;
} __attribute__ ((packed)) RtcPcf85063aOffset_t;

typedef struct
{
    uint8_t seconds         : 7;
    uint8_t os              : 1;
} __attribute__ ((packed)) RtcPcf85063aSeconds_t;

typedef struct
{
    uint8_t minutes         : 7;
    uint8_t                 : 1;
} __attribute__ ((packed)) RtcPcf85063aMinutes_t;

typedef struct
{
    uint8_t hours           : 6;
    uint8_t                 : 2;
} __attribute__ ((packed)) RtcPcf85063aHours_t;

typedef struct
{
    uint8_t days            : 6;
    uint8_t                 : 2;
} __attribute__ ((packed)) RtcPcf85063aDays_t;

typedef struct
{
    uint8_t weekdays        : 3;
    uint8_t                 : 5;
} __attribute__ ((packed)) RtcPcf85063aWeekdays_t;

typedef struct
{
    uint8_t months          : 5;
    uint8_t                 : 3;
} __attribute__ ((packed)) RtcPcf85063aMonths_t;

typedef union
{
    RtcPcf85063aControl_1_t control1Reg;
    RtcPcf85063aControl_2_t control2Reg;
    RtcPcf85063aOffset_t offsetReg;
    RtcPcf85063aSeconds_t secondsReg;
    RtcPcf85063aMinutes_t minutesReg;
    RtcPcf85063aHours_t hoursReg;
    RtcPcf85063aDays_t daysReg;
    RtcPcf85063aWeekdays_t weekdaysReg;
    RtcPcf85063aMonths_t monthsReg;

    uint8_t byte;
}RtcPcf85063aReg_t;

/*****************************************************************************
                     PRIVATE VARIABLES
*****************************************************************************/

static const char* TAG = "rtc";

static i2c_port_t sI2cPortNumber = I2C_PORT_NUMBER;

static bool sRtcError;

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief  Read multiple data bytes from a rtc
 *  @param registerAddress register address from which we want to start reading data
 *  @param data pointer to read data
 *  @param length number bytes to read
 *  @return return true if success
 */
static bool Pcf85063aBlockRead(RtcPcf85063aRegAddr_t registerAddress, uint8_t* data, uint8_t length);

/** @brief  Write multiple data bytes to a rtc
 *  @param registerAddress register address from which we want to start writting data
 *  @param data pointer to write data
 *  @param length number bytes to write
 *  @return return true if success
 */
static bool Pcf85063aBlockWrite(RtcPcf85063aRegAddr_t registerAddress, uint8_t* data, uint8_t length);

/** @brief  Convert decimal value to bcd value
 *  @param decVal decimal value
 *  @return return bcd value
 */
static uint8_t dec2bcd(uint8_t decVal);

/** @brief  Convert bcd value to decimal value 
 *  @param bcdVal bcd value
 *  @return return decimal value
 */
static uint8_t bcd2dec(uint8_t bcdVal);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool RtcDriverInit(void)
{   
    bool res = true;
    uint8_t yearReg = 0;
 
    res &= Pcf85063aBlockRead(PCF85063A_YEARS, &yearReg, 1);
    uint8_t year = bcd2dec(yearReg); // value from 0 to 99
    ESP_LOGI(TAG, "read year %d", (year + 2000U));

    if((year + 2000U) < RTC_IS_CORRECT_YEAR_SET){
        sRtcError = true;

        ESP_LOGE(TAG, "read incorrect year %d", (year + 2000U));
        ESP_LOGE(TAG, "something is wrong with rtc battery");
    }

    SettingDevice_t setting = {};
    iotHubClientStatus_t clientStatus = {};
    struct tm rtcTime = {};

    res &= SettingGet(&setting);
    res &= RtcDriverGetDateTime(&rtcTime);
    res &= IotHubClientGetSetting(&clientStatus);

    time_t rtcUnix = mktime(&rtcTime);

    uint16_t deltaTime = DEFAULT_RTC_CORRECTION_TIME_SEC;
    if(clientStatus.lastConnection != 0){
        double timeWithoutConnection = (double)setting.restore.saveTimestamp - (double)clientStatus.lastConnection;
        timeWithoutConnection *= RTC_CRISTAL_PPM;
        timeWithoutConnection /= 1000000.0f;
        
        if(timeWithoutConnection > 0){
            deltaTime += (uint16_t)timeWithoutConnection;
        }
        else{
            deltaTime -= (uint16_t)timeWithoutConnection;
        }
    }

    ESP_LOGI(TAG, "delta time %u", deltaTime);

    if((rtcUnix + deltaTime) < setting.restore.saveTimestamp){
        sRtcError = true;

        ESP_LOGE(TAG, "From rtc %u read from setting %u", (uint32_t)rtcUnix, setting.restore.saveTimestamp);
        ESP_LOGE(TAG, "nvs saved time is bigger than actual rtc time");
        ESP_LOGE(TAG, "something is wrong with rtc battery");
    }

    return res;
}

bool RtcDriverSetDateTime(struct tm *time)
{
    uint8_t rtcData[7] = {};
    RtcPcf85063aReg_t pcf85063aReg = {};

    // cppcheck-suppress unreadVariable
    pcf85063aReg.secondsReg.seconds = dec2bcd(time->tm_sec);
    rtcData[0] = pcf85063aReg.byte;

    pcf85063aReg.byte = 0;
    // cppcheck-suppress unreadVariable
    pcf85063aReg.minutesReg.minutes = dec2bcd(time->tm_min);
    rtcData[1] = pcf85063aReg.byte;

    pcf85063aReg.byte = 0;
    // cppcheck-suppress unreadVariable
    pcf85063aReg.hoursReg.hours = dec2bcd(time->tm_hour);
    rtcData[2] = pcf85063aReg.byte;

    pcf85063aReg.byte = 0;
    // cppcheck-suppress unreadVariable
    pcf85063aReg.daysReg.days = dec2bcd(time->tm_mday);
    rtcData[3] = pcf85063aReg.byte;

    pcf85063aReg.byte = 0;
    // cppcheck-suppress unreadVariable
    pcf85063aReg.weekdaysReg.weekdays = dec2bcd(time->tm_wday);
    rtcData[4] = pcf85063aReg.byte;

    pcf85063aReg.byte = 0;
    // cppcheck-suppress unreadVariable
    pcf85063aReg.monthsReg.months = dec2bcd(time->tm_mon + 1);
    rtcData[5] = pcf85063aReg.byte;

    // tm_year -> years since 1900
    // rtc year reg. -> BTC format (0 to 99)
    pcf85063aReg.byte = dec2bcd(time->tm_year - 100);
    rtcData[6] = pcf85063aReg.byte;

    sRtcError = false; // we clean the flag because the time in RTC is always set by the user (webserwer or cloud)

    return Pcf85063aBlockWrite(PCF85063A_SECONDS, rtcData, 7);
}

bool RtcDriverGetDateTime(struct tm *time)
{
    uint8_t rtcData[7] = {};
    RtcPcf85063aReg_t pcf85063aReg = {};

    bool res = Pcf85063aBlockRead(PCF85063A_SECONDS, rtcData, 7);
    if(res == false){
        return false;
    }

    // cppcheck-suppress redundantAssignment
    pcf85063aReg.byte = rtcData[0];
    time->tm_sec = bcd2dec(pcf85063aReg.secondsReg.seconds);

    // cppcheck-suppress redundantAssignment
    pcf85063aReg.byte = rtcData[1];
    time->tm_min = bcd2dec(pcf85063aReg.minutesReg.minutes);

    // cppcheck-suppress redundantAssignment
    pcf85063aReg.byte = rtcData[2];
    time->tm_hour = bcd2dec(pcf85063aReg.hoursReg.hours);

    // cppcheck-suppress redundantAssignment
    pcf85063aReg.byte = rtcData[3];
    time->tm_mday = bcd2dec(pcf85063aReg.daysReg.days);

    // cppcheck-suppress redundantAssignment
    pcf85063aReg.byte = rtcData[4];
    time->tm_wday = bcd2dec(pcf85063aReg.weekdaysReg.weekdays);

    // cppcheck-suppress redundantAssignment
    pcf85063aReg.byte = rtcData[5];
    pcf85063aReg.monthsReg.months = pcf85063aReg.monthsReg.months - 1;
    time->tm_mon = bcd2dec(pcf85063aReg.monthsReg.months);

    // cppcheck-suppress redundantAssignment
    pcf85063aReg.byte = bcd2dec(rtcData[6]);
    pcf85063aReg.byte = pcf85063aReg.byte + 100;
    time->tm_year = pcf85063aReg.byte;

    return true;
}

bool RtcDriverIsError(void)
{
    return sRtcError;
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool Pcf85063aBlockRead(RtcPcf85063aRegAddr_t registerAddress, uint8_t* data, uint8_t length)
{
    if((data == NULL) || (length == 0)){
        return false;
    }
    
    bool res = true;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    /*
     * _____________________________________________________________
     * | start | slave_addr + wr_bit + ack | reg_addr + ack  | stop |
     * --------|---------------------------|-----------------|------|
    */
    esp_err_t esp_res = i2c_master_start(cmd);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write_byte(cmd, PCF85063A_SLAVE_ADDRESS << 1 | I2C_MASTER_WRITE, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write_byte(cmd, registerAddress, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_stop(cmd);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_cmd_begin(sI2cPortNumber, cmd, PCF85063A_READ_TIMEOUT_MS / portTICK_RATE_MS);
    if(esp_res != ESP_OK){
        res &= false;
    }

    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    /*
     * ______________________________________________________________________________________________________
     * | start | slave_addr + rd_bit + ack | reg_data + ack  | reg_data + ack | ... | reg_data + nack | stop |
     * --------|---------------------------|-----------------|----------------|-----|-----------------|------|
    */
    esp_res = i2c_master_start(cmd);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write_byte(cmd, PCF85063A_SLAVE_ADDRESS << 1 | I2C_MASTER_READ, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_read(cmd, data, length, I2C_MASTER_LAST_NACK);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_stop(cmd);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_cmd_begin(sI2cPortNumber, cmd, PCF85063A_READ_TIMEOUT_MS / portTICK_RATE_MS);
    if(esp_res != ESP_OK){
        res &= false;
    }

    i2c_cmd_link_delete(cmd);

    return res;
}

static bool Pcf85063aBlockWrite(RtcPcf85063aRegAddr_t registerAddress, uint8_t* data, uint8_t length)
{
    if((data == NULL) || (length == 0)){
        return false;
    }

    bool res = true;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    /*
     * _____________________________________________________________________________________________________
     * | start | slave_addr + wr_bit + ack | reg_addr + ack  | reg_data + ack | ... | reg_data + ack | stop |
     * --------|---------------------------|-----------------|----------------|-----|----------------|------|
    */
    esp_err_t esp_res = i2c_master_start(cmd);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write_byte(cmd, PCF85063A_SLAVE_ADDRESS << 1 | I2C_MASTER_WRITE, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write_byte(cmd, registerAddress, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write(cmd, data, length, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_stop(cmd);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_cmd_begin(sI2cPortNumber, cmd, PCF85063A_WTITE_TIMEOUT_MS / portTICK_RATE_MS);
    if(esp_res != ESP_OK){
        res &= false;
    }

    i2c_cmd_link_delete(cmd);

    return res;
}

static uint8_t dec2bcd(uint8_t decVal)
{
    return ((decVal / 10) << 4) + (decVal % 10);
}

static uint8_t bcd2dec(uint8_t bcdVal)
{
    return (bcdVal >> 4) * 10 + (bcdVal & 0x0f);
}