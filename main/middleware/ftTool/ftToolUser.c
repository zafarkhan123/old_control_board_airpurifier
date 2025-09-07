/**
 * @file ftToolUser.c
 *
 * @brief Ft tool user middleware source file
 *
 * @dir ftToolUser
 * @brief Fan middleware folder
 *
 * @author matfio
 * @date 2021.10.19
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "config.h"
#include "setting.h"

#include "fan/fan.h"
#include "ledDriver/ledDriver.h"
#include "uvLamp/uvLamp.h"

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Read selected color componenets
 *  @param channel channel number
 *  @param dataPtr [out] pointer to out buffer
 *  @param dataSize data size
 *  @param colorName slected color
 *  @return return true if success
 */
static bool ReadColorComponent(uint8_t channel, void *dataPtr, uint32_t dataSize, LedDriverColor_t colorName);

/** @brief Write new color componenets
 *  @param channel channel number
 *  @param dataPtr [in] pointer to in buffer
 *  @param dataSize data size
 *  @param colorName slected color
 *  @return return true if success
 */
static bool WriteColorComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize, LedDriverColor_t colorName);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool FtToolUserReadPwmFanLevel(uint8_t channel, void *dataPtr, uint32_t dataSize)
{
     uint16_t pwm = FanGetActualPwmFanLevel(channel);

    *(uint16_t *)dataPtr = pwm;

    return true;
}

bool FtToolUserWritePwmFanLevel(uint8_t channel, const void *dataPtr, uint32_t dataSize)
{
    uint16_t newPwm = *(uint16_t *)dataPtr;

    FanSetNewPwmFanLevel(channel, newPwm);
    return true;
}

bool FtToolUserReadColorOffComponent(uint8_t channel, void *dataPtr, uint32_t dataSize)
{   
    return ReadColorComponent(channel, dataPtr, dataSize, LED_COLOR_OFF);
}

bool FtToolUserWriteColorOffComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize)
{
    return WriteColorComponent(channel, dataPtr, dataSize, LED_COLOR_OFF);
}

bool FtToolUserReadColorWhiteComponent(uint8_t channel, void *dataPtr, uint32_t dataSize)
{
    return ReadColorComponent(channel, dataPtr, dataSize, LED_COLOR_WHITE);
}

bool FtToolUserWriteColorWhiteComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize)
{
    return WriteColorComponent(channel, dataPtr, dataSize, LED_COLOR_WHITE);
}

bool FtToolUserReadColorRedComponent(uint8_t channel, void *dataPtr, uint32_t dataSize)
{
    return ReadColorComponent(channel, dataPtr, dataSize, LED_COLOR_RED);
}

bool FtToolUserWriteColorRedComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize)
{
    return WriteColorComponent(channel, dataPtr, dataSize, LED_COLOR_RED);
}

bool FtToolUserReadColorGreenComponent(uint8_t channel, void *dataPtr, uint32_t dataSize)
{
    return ReadColorComponent(channel, dataPtr, dataSize, LED_COLOR_GREEN);
}

bool FtToolUserWriteColorGreenComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize)
{
    return WriteColorComponent(channel, dataPtr, dataSize, LED_COLOR_GREEN);
}

bool FtToolUserReadColorBlueComponent(uint8_t channel, void *dataPtr, uint32_t dataSize)
{
    return ReadColorComponent(channel, dataPtr, dataSize, LED_COLOR_BLUE);
}

bool FtToolUserWriteColorBlueComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize)
{
    return WriteColorComponent(channel, dataPtr, dataSize, LED_COLOR_BLUE);
}

bool FtToolUserReadColorOrangeComponent(uint8_t channel, void *dataPtr, uint32_t dataSize)
{
    return ReadColorComponent(channel, dataPtr, dataSize, LED_COLOR_ORANGE);
}

bool FtToolUserWriteColorOrangeComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize)
{
    return WriteColorComponent(channel, dataPtr, dataSize, LED_COLOR_ORANGE);
}

bool FtToolUserReadTachoSpeed(uint8_t channel, void *dataPtr, uint32_t dataSize)
{
    int16_t revolutions = 0;
    FanGetTachoRevolutionsPerSecond(&revolutions);

    *(uint16_t *)dataPtr = revolutions;

    return true;
}

bool FtToolUserReadUvLampVoltage(uint8_t channel, void *dataPtr, uint32_t dataSize)
{
    uint32_t voltage = 0;

    if(channel == 0){
        voltage = UvLampGetMeanMiliVolt(UV_LAMP_1);
    }
    else if(channel == 1){
        voltage = UvLampGetMeanMiliVolt(UV_LAMP_2);
    }

    *(uint32_t *)dataPtr = voltage; 

    return true;
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool ReadColorComponent(uint8_t channel, void *dataPtr, uint32_t dataSize, LedDriverColor_t colorName)
{
    uint8_t data = 0;
    rgb_t rgb = LedDriverGetColorComponents(colorName);

    if(channel == 0){
        data = rgb.r;
    }
    else if(channel == 1){
        data = rgb.g;
    }
    else if(channel == 2){
        data = rgb.b;
    }

    *(uint8_t *)dataPtr = data;

    return true;
}

static bool WriteColorComponent(uint8_t channel, const void *dataPtr, uint32_t dataSize, LedDriverColor_t colorName)
{
    uint8_t data = *(uint8_t *)dataPtr;

    rgb_t rgb = LedDriverGetColorComponents(colorName);

    if(channel == 0){
        rgb.r = data;
    }
    else if(channel == 1){
        rgb.g = data;
    }
    else if(channel == 2){
        rgb.b = data;
    }

    LedDriverSetColorComponents(colorName, rgb);

    return true;
}