/**
 * @file touch.c
 *
 * @brief Touch middleware source file
 *
 * @dir touch
 * @brief Touch middleware folder
 *
 * @author matfio
 * @date 2021.08.30
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "config.h"

#include <stdio.h>
#include <sys/time.h>
#include <esp_log.h>

#include "touch.h"

#include "touchDriver/touchDriver.h"
#include "timeDriver/timeDriver.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define SHORT_PRESS_TIME_MS (250U)
#define LONG_PRESS_TIME_MS (2U * 1000U)
#define VERY_LONG_PRESS_TIME_MS (10U * 1000U)

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

typedef struct{
    int64_t risingTime;
    int64_t fallingTime;
    uint8_t isPress : 1;
    uint8_t isReleas : 1;
    uint8_t shortPressDetected : 1;
    uint8_t longPressDetected : 1;
    uint8_t veryLongPressDetected : 1;
}__attribute__ ((packed)) touchStatus_t;

static const char* TAG = "touch";

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool TouchInit(void)
{
    bool res = true;
    TouchDriverInfo_t deviceInfo = {};

    res &= TouchDriverInit();
    res &= TouchDriverGetDeviceInfo(&deviceInfo);

    ESP_LOGI(TAG, "Touch Product ID %X, Manufacturer ID %X, Revision %X\n", deviceInfo.productId, deviceInfo.manufacturedId, deviceInfo.revision);
    
    return res;
}

bool TouchButtonStatus(TouchButtons_t* buttons)
{
    static touchStatus_t sTouchStatus[CFG_TOUCH_BUTTON_NAME_COUNT] = {
        [CFG_TOUCH_BUTTON_NAME_POWER] = {.isReleas = true},
        [CFG_TOUCH_BUTTON_NAME_FAN_INC] = {.isReleas = true},
        [CFG_TOUCH_BUTTON_NAME_FAN_DEC] = {.isReleas = true},
    };

    TouchDriverButtonStatus_t buttonStatus = {};

    TouchDriverInputStatus_t buttonReadStatus = TouchDriverIsButtonTouched(&buttonStatus);
    if(buttonReadStatus == TOUCH_INPUT_STATUS_ERROR){
        ESP_LOGE(TAG, "TouchDriverIsButtonTouched read error");
    }

    uint16_t idx;
    bool isAnyButtonPress = false;

    for(idx = 0; idx < CFG_TOUCH_BUTTON_NAME_COUNT; ++idx)
    {
        if((buttonReadStatus == TOUCH_INPUT_STATUS_CHANGES_DETECTED) || (sTouchStatus[idx].isPress == true) || (sTouchStatus[idx].isReleas == false)){
            isAnyButtonPress = true;
        }
    }

    if(isAnyButtonPress == false){
        return false;
    }

    if(buttonReadStatus == TOUCH_INPUT_STATUS_CHANGES_DETECTED){

        for(idx = 0; idx < CFG_TOUCH_BUTTON_NAME_COUNT; ++idx)
        {
            if(buttonStatus.isPressNow[idx] == true){
                sTouchStatus[idx].risingTime = TimeDriverGetSystemTickMs();
                sTouchStatus[idx].isPress = true;

                ESP_LOGI(TAG, "pressing detected %d", idx);
            }

            if((buttonStatus.isPressNow[idx] == false) && (sTouchStatus[idx].isPress == true)){
                sTouchStatus[idx].fallingTime = TimeDriverGetSystemTickMs();
                sTouchStatus[idx].isReleas = true;
                sTouchStatus[idx].isPress = false;

                sTouchStatus[idx].shortPressDetected = false;
                sTouchStatus[idx].longPressDetected = false;
                sTouchStatus[idx].veryLongPressDetected = false;

                ESP_LOGI(TAG, "release detected %d", idx);
            }
        } 
    }

    bool isPressLongEnought = false;
    for(idx = 0; idx < CFG_TOUCH_BUTTON_NAME_COUNT; ++idx)
    {
        if(sTouchStatus[idx].isPress == true){
            buttons->status[idx] = TOUCH_BUTTON_PRESS_NO;

            int64_t delaTime = TimeDriverGetSystemTickMs() - sTouchStatus[idx].risingTime;
        
            if(delaTime > VERY_LONG_PRESS_TIME_MS){
                if(sTouchStatus[idx].veryLongPressDetected == false){
                    buttons->status[idx] = TOUCH_BUTTON_PRESS_VERY_LONG;
                    sTouchStatus[idx].veryLongPressDetected = true;
                    
                    isPressLongEnought = true;
                    ESP_LOGI(TAG, "very long %d", idx);
                }

                 
            }else if(delaTime > LONG_PRESS_TIME_MS){
                if(sTouchStatus[idx].longPressDetected == false){
                    buttons->status[idx] = TOUCH_BUTTON_PRESS_LONG;
                    sTouchStatus[idx].longPressDetected = true;

                    isPressLongEnought = true;
                    ESP_LOGI(TAG, "long %d", idx);
                }

            }else if(delaTime > SHORT_PRESS_TIME_MS){
                if(sTouchStatus[idx].shortPressDetected == false){
                    buttons->status[idx] = TOUCH_BUTTON_PRESS_SHORT;
                    sTouchStatus[idx].shortPressDetected = true;

                    isPressLongEnought = true;
                    ESP_LOGI(TAG, "short %d", idx);
                }

            }else{
                buttons->status[idx] = TOUCH_BUTTON_PRESS_NO;
            }
        }
    }

    return isPressLongEnought;
}

bool TouchChangeDeviceSetting(SettingDevice_t* settingDevice, TouchButtons_t* buttons)
{
    if(buttons->status[CFG_TOUCH_BUTTON_NAME_POWER] == TOUCH_BUTTON_PRESS_SHORT){
        if(settingDevice->restore.deviceStatus.isDeviceOn == true){
            settingDevice->restore.deviceStatus.isDeviceOn = false;
            settingDevice->restore.deviceStatus.fanLevel = FAN_LEVEL_1; // the app does the same
        }
        else{
            settingDevice->restore.deviceStatus.isDeviceOn = true;
        }
        buttons->status[CFG_TOUCH_BUTTON_NAME_POWER] = TOUCH_BUTTON_PRESS_NO;
        ESP_LOGI(TAG, "on/off press");
    }

    // don't want to change the fan level speed when device is turned off 
    if(settingDevice->restore.deviceStatus.isDeviceOn == false){
        return true;
    }

    if(buttons->status[CFG_TOUCH_BUTTON_NAME_FAN_INC] == TOUCH_BUTTON_PRESS_SHORT){
        if(settingDevice->restore.deviceStatus.fanLevel != FAN_LEVEL_5){
            ++settingDevice->restore.deviceStatus.fanLevel;
        }

        buttons->status[CFG_TOUCH_BUTTON_NAME_FAN_INC] = TOUCH_BUTTON_PRESS_NO;
        ESP_LOGI(TAG, "fan+ press");
    }

    if(buttons->status[CFG_TOUCH_BUTTON_NAME_FAN_DEC] == TOUCH_BUTTON_PRESS_SHORT){
        if(settingDevice->restore.deviceStatus.fanLevel != FAN_LEVEL_1){
            --settingDevice->restore.deviceStatus.fanLevel;
        }

        buttons->status[CFG_TOUCH_BUTTON_NAME_FAN_DEC] = TOUCH_BUTTON_PRESS_NO;
        ESP_LOGI(TAG, "fan- press");
    }

    return true;
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/