/**
 * @file touchDriver.c
 *
 * @brief Touch driver source file
 *
 * @dir touchDriver
 * @brief Touch driver folder
 *
 * @author matfio
 * @date 2021.08.25
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "touchDriver.h"

#include <string.h>

#include "esp_log.h"

#include "driver/i2c.h"
#include "hal/i2c_types.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define I2C_PORT_NUMBER (CFG_I2C_PORT_NUMBER)

#define CAPxxxx_ADDRESS (0b0101000)

#define CAPxxxx_WTITE_TIMEOUT_MS (1000U)
#define CAPxxxx_READ_TIMEOUT_MS (1000U)

#define UNUSED(x) ((void)(x))

/*****************************************************************************
                     PRIVATE ENUMS
*****************************************************************************/

typedef enum 
{
	MAIN_CONTROL = 0x00,
	GENERAL_STATUS = 0x02,
	SENSOR_INPUT_STATUS = 0x03,
	NOISE_FLAG_STATUS = 0x0A,
	SENSOR_INPUT_1_DELTA_COUNT = 0x10,
	SENSOR_INPUT_2_DELTA_COUNT = 0X11,
	SENSOR_INPUT_3_DELTA_COUNT = 0X12,
	SENSITIVITY_CONTROL = 0x1F,
	CONFIG = 0x20,
	SENSOR_INPUT_ENABLE = 0x21,
	SENSOR_INPUT_CONFIG = 0x22,
	SENSOR_INPUT_CONFIG_2 = 0x23,
	AVERAGING_AND_SAMPLE_CONFIG = 0x24,
	CALIBRATION_ACTIVATE_AND_STATUS = 0x26,
	INTERRUPT_ENABLE = 0x27,
	REPEAT_RATE_ENABLE = 0x28,
	MULTIPLE_TOUCH_CONFIG = 0x2A,
	MULTIPLE_TOUCH_PATTERN_CONFIG = 0x2B,
	MULTIPLE_TOUCH_PATTERN = 0x2D,
	BASE_COUNT_OUT = 0x2E,
	RECALIBRATION_CONFIG = 0x2F,
	SENSOR_1_INPUT_THRESH = 0x30,
	SENSOR_2_INPUT_THRESH = 0x31,
	SENSOR_3_INPUT_THRESH = 0x32,
	SENSOR_INPUT_NOISE_THRESH = 0x38,
	STANDBY_CHANNEL = 0x40,
	STANDBY_CONFIG = 0x41,
	STANDBY_SENSITIVITY = 0x42,
	STANDBY_THRESH = 0x43,
	CONFIG_2 = 0x44,
	SENSOR_INPUT_1_BASE_COUNT = 0x50,
	SENSOR_INPUT_2_BASE_COUNT = 0x51,
	SENSOR_INPUT_3_BASE_COUNT = 0x52,
	POWER_BUTTON = 0x60,
	POWER_BUTTON_CONFIG = 0x61,
	SENSOR_INPUT_1_CALIBRATION = 0xB1,
	SENSOR_INPUT_2_CALIBRATION = 0xB2,
	SENSOR_INPUT_3_CALIBRATION = 0xB3,
	SENSOR_INPUT_CALIBRATION_LSB_1 = 0xB9,
	PROD_ID = 0xFD,
	MANUFACTURE_ID = 0xFE,
	REVISION = 0xFF,
}TouchCap1293RegAddr_t;

typedef enum{
    TOUCH_SENSITIVITY_128X =    0x00,   // Most sensitive
    TOUCH_SENSITIVITY_64X =     0x01,
    TOUCH_SENSITIVITY_32X =     0x02,
    TOUCH_SENSITIVITY_16X =     0x03,
    TOUCH_SENSITIVITY_8X =      0x04,
    TOUCH_SENSITIVITY_4X =      0x05,
    TOUCH_SENSITIVITY_2X =      0x06,
    TOUCH_SENSITIVITY_1X =      0x01,   // Least sensitive
}TouchDriverSensitivity_t;

typedef enum{
    TOUCH_POWER_BUTTON_INPUT_SENSOR_CS1 = 0,
    TOUCH_POWER_BUTTON_INPUT_SENSOR_CS2 = 1,
    TOUCH_POWER_BUTTON_INPUT_SENSOR_CS3 = 2,
}TouchDriverPowerButtonInputSensor_t;

typedef enum{
    TOUCH_POWER_BUTTON_HOLD_TIME_280_MS =   0x00,   // 280 [mS]
    TOUCH_POWER_BUTTON_HOLD_TIME_560_MS =   0x01,   // 560 [mS]
    TOUCH_POWER_BUTTON_HOLD_TIME_1120_MS =  0x02,   // 1.12 [S]
    TOUCH_POWER_BUTTON_HOLD_TIME_2240_MS =  0x03    // 2.24 [S]
}TouchDriverPowerButtonHoldTime_t;

/*****************************************************************************
                     PRIVATE STRUCTS
*****************************************************************************/

// Main Control Register 0x00
typedef struct
{
	uint8_t Int             : 1;
	uint8_t combo           : 1;
    uint8_t cGain           : 2;
	uint8_t dSleep          : 1;
	uint8_t stby            : 1;
	uint8_t gain            : 2;
} __attribute__ ((packed)) Cap1293MainControlReg_t;

// General Status Reg 0x02
typedef struct
{
	uint8_t touch           : 1;
	uint8_t mtp             : 1;
	uint8_t mult            : 1;
	uint8_t                 : 1;
	uint8_t pwr             : 1;
	uint8_t acalFail        : 1;
	uint8_t bcOut           : 1;
	uint8_t                 : 1;
} __attribute__ ((packed)) Cap1293GeneralStatusReg_t;

// Sensor Input Status Reg 0x03
typedef struct
{
	uint8_t cs1             : 1;
	uint8_t cs2             : 1;
	uint8_t cs3             : 1;
	uint8_t                 : 5;
} __attribute__ ((packed)) Cap1293SensorInputStatusReg_t;

// Calibration Activate and Status Register 0x26, default value is 0x00
typedef struct
{
	uint8_t cs1Cal          : 1;
	uint8_t cs2Cal          : 1;
	uint8_t cs3Cal          : 1;
	uint8_t                 : 5;
} __attribute__ ((packed)) Cap1293CalibrationActivateAndStatusReg_t;

// Interrupt Enable Register 0x27
typedef struct
{
	uint8_t cs1IntEn        : 1;
	uint8_t cs2IntEn        : 1;
	uint8_t cs3IntEn        : 1;
	uint8_t                 : 5;

} __attribute__ ((packed)) Cap1293InterruptEnableReg_t;

// Repeat Rate ENABLE Register 0x28, default value is 0x07
typedef struct
{
	uint8_t cs1RptEn        : 1;
	uint8_t cs2RptEn        : 1;
	uint8_t cs3RptEn        : 1;
	uint8_t                 : 5;
} __attribute__ ((packed)) Cap1293RepeatRateEnableReg_t;

// Standby Channel Register 0x40, default value is 0x00
typedef struct
{
	uint8_t cs1STBY         : 1;
	uint8_t cs2STBY         : 1;
	uint8_t cs3STBY         : 1;
	uint8_t                 : 5;
} __attribute__ ((packed)) Cap1293StandbyChannelReg_t;

// Power Button Register 0x60
typedef struct
{
	uint8_t pwrBtn          : 3;
	uint8_t                 : 5;
} __attribute__ ((packed)) Cap1293PowerButtonReg_t;

// Power Button Configuration Register 0x61
typedef struct
{
	uint8_t pwrTime         : 2;
	uint8_t pwrEn           : 1;
	uint8_t                 : 1;
	uint8_t stbyPwrTime     : 2;
	uint8_t stbyPwrEn       : 1;
	uint8_t                 : 1;
} __attribute__ ((packed)) Cap1293PowerButtonConfigReg_t;

// Sensitivity Control Reg (0x1F)
typedef struct
{
	uint8_t baseShift       : 4;
	uint8_t deltaSense      : 3;
	uint8_t                 : 1;
} __attribute__ ((packed)) Cap1293SensitivityControlReg_t;

//  Multiple Touch Configuration Register 0x2a
typedef struct
{
	uint8_t                 : 2;
	uint8_t bMultT          : 2;
	uint8_t                 : 3;
	uint8_t multBlkEn       : 1;
} __attribute__ ((packed)) Cap1293MtpConfigReg_t;

// MTP Enable Register 0x2b
typedef struct
{
	uint8_t mtpAlert        : 1;
	uint8_t compPtrn        : 1;
	uint8_t mtpTh           : 2;
	uint8_t                 : 3;
	uint8_t mtpEn           : 1;
} __attribute__ ((packed)) Cap1293MtpEnableReg_t;

// MTP Enable Register 0x2d
typedef struct
{
	uint8_t cs1Ptrn         : 1;
	uint8_t cs2Ptrn         : 1;
	uint8_t cs3Ptrn         : 1;
	uint8_t                 : 5;
} __attribute__ ((packed)) Cap1293MultipleTouchPatternReg_t;

typedef union{
    Cap1293SensitivityControlReg_t sensitivityControl;
    Cap1293GeneralStatusReg_t generalStatus;
    Cap1293SensorInputStatusReg_t sensorInputStatus;
    Cap1293MainControlReg_t mainControl;
    Cap1293PowerButtonReg_t powerButton;
    Cap1293PowerButtonConfigReg_t powerButtonConfig;
    Cap1293InterruptEnableReg_t interruptEnable;
    Cap1293CalibrationActivateAndStatusReg_t calibrationActivateAndStatus;
    Cap1293RepeatRateEnableReg_t repeatRateEnable;
    Cap1293MtpConfigReg_t mtpConfig;
    Cap1293MtpEnableReg_t mtpEnable;
    Cap1293MultipleTouchPatternReg_t multipleTouchPattern;
    Cap1293StandbyChannelReg_t standbyChannel;

	uint8_t byte;
}Cap1293Reg_t;

/*****************************************************************************
                     PRIVATE VARIABLES
*****************************************************************************/

static i2c_port_t sI2cPortNumber = I2C_PORT_NUMBER;

static const char* TAG = "touchDr";

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief  Read multiple data bytes from a group of contiguous registers
 *  @param registerAddress register address from which we want to start reading data
 *  @param data pointer to read data
 *  @param length number bytes to read
 *  @return return true if success
 */
static bool CapxxxxBlockRead(TouchCap1293RegAddr_t registerAddress, uint8_t* data, uint8_t length);

/** @brief  Write multiple data bytes to a group of contiguous registers
 *  @param registerAddress register address from which we want to start writting data
 *  @param data pointer to write data
 *  @param length number bytes to write
 *  @return return true if success
 */
static bool CapxxxxBlockWrite(TouchCap1293RegAddr_t registerAddress, uint8_t* data, uint8_t length);

/** @brief Turn on Auto calibration (recommendet after power on)
 *  @return return true if success
 */
static bool CapxxxxForceCalibrateEnabled(void);

/** @brief  Set touchpad sensitivity
 *  @param sensitivity touch sensitivity
 *  @return return true if success
 */
static bool CapxxxxSetSensitivity(TouchDriverSensitivity_t sensitivity);

/** @brief  Enable multi touch
 *  @return return true if success
 */
static bool CapxxxxSetMultiTouchEnabled(void);

/** @brief  Disable repeat rate
 *  @return return true if success
 */
static bool CapxxxxSetRepeatRateDisabled(void);

/** @brief  Initialize Alert pin as input
 *  @return return true if success
 */
static bool GpioAlertPinInit(void);

/** @brief  Clear INT flat in Main Control reg.
 *  @return return true if success
 */
static bool CapxxxxClearIntFlag(void);

/** @brief  Get input status
 *  @param buttoStatus [out] pointer to button status struct TouchDriverButtonStatus_t
 *  @return return true if success
 */
static bool CapxxxxGetInputStatus(TouchDriverButtonStatus_t* buttoStatus);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool TouchDriverInit(void)
{
    bool res = true;

    res &= CapxxxxSetMultiTouchEnabled();
    res &= CapxxxxSetSensitivity(TOUCH_SENSITIVITY_32X);
    res &= CapxxxxSetRepeatRateDisabled();
    res &= CapxxxxForceCalibrateEnabled();
    res &= TouchDriverSetPowerState(TOUCH_POWER_STATE_ACTICE);

    res &= GpioAlertPinInit();

    return res;
}

bool TouchDriverGetDeviceInfo(TouchDriverInfo_t* deviceInfo)
{
    return CapxxxxBlockRead(PROD_ID, &(deviceInfo->productId) , 3);
}

TouchDriverInputStatus_t TouchDriverIsButtonTouched(TouchDriverButtonStatus_t* buttonStatus)
{
    bool res = true;

    if(TouchDriverIsAlertSet() == false){
        return TOUCH_INPUT_STATUS_NOTHING_DETECTED;
    }
    
    res &= CapxxxxClearIntFlag();
    res &= CapxxxxGetInputStatus(buttonStatus);

    if(res == false){
        return TOUCH_INPUT_STATUS_ERROR;
    }
    
    return TOUCH_INPUT_STATUS_CHANGES_DETECTED;
}

bool TouchDriverIsAlertSet(void)
{
    if(gpio_get_level(CFG_TOUCH_INTERRUPT_PIN)){
        return false;
    }

    return true;
}

bool TouchDriverSetPowerState(TouchDriverPowerState_t powerState)
{
    bool res = true;
    Cap1293Reg_t reg = {0};

    reg.byte = 0;
    res &= CapxxxxBlockRead(MAIN_CONTROL, &reg.byte, 1);

    ESP_LOGI(TAG, "power state %d", powerState);
    
    switch (powerState)
    {
        case TOUCH_POWER_STATE_ACTICE:{
            reg.mainControl.stby = 0;
            reg.mainControl.combo = 0;
            reg.mainControl.dSleep = 0;
            break;
        }

        case TOUCH_POWER_STATE_STANBY:{
            // Power button work in standby mode
            reg.mainControl.stby = 1;
            reg.mainControl.combo = 0;
            reg.mainControl.dSleep = 0;
            break;
        }
        case TOUCH_POWER_STATE_COMBO:{
            reg.mainControl.combo = 1;
            reg.mainControl.dSleep = 0;
            break;
        }
        case TOUCH_POWER_STATE_DSLEEP:{
            reg.mainControl.dSleep = 1;
            break;
        }
        default:{
            ESP_LOGE(TAG, "unknown power state %d", powerState);
            break;
        }
    }

    res &= CapxxxxBlockWrite(MAIN_CONTROL, &reg.byte, 1);

    return res;
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool CapxxxxBlockRead(TouchCap1293RegAddr_t registerAddress, uint8_t* data, uint8_t length)
{
    if((data == NULL) || (length == 0)){
        return false;
    }
    
    bool res = true;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    /*
     * ______________________________________________________
     * | start | slave_addr + wr_bit + ack | reg_addr + ack  |
     * --------|---------------------------|-----------------|
    */
    esp_err_t esp_res = i2c_master_start(cmd);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write_byte(cmd, CAPxxxx_ADDRESS << 1 | I2C_MASTER_WRITE, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write_byte(cmd, registerAddress, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    /*
     * ______________________________________________________________________________________________________
     * | start | slave_addr + rd_bit + ack | reg_data + ack  | reg_data + ack | ... | reg_data + nack | stop |
     * --------|---------------------------|-----------------|----------------|-----|-----------------|------|
    */
    esp_res = i2c_master_start(cmd);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write_byte(cmd, CAPxxxx_ADDRESS << 1 | I2C_MASTER_READ, true);
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

    esp_res = i2c_master_cmd_begin(sI2cPortNumber, cmd, CAPxxxx_READ_TIMEOUT_MS / portTICK_RATE_MS);
    if(esp_res != ESP_OK){
        res &= false;
    }

    i2c_cmd_link_delete(cmd);

    return res;
}

static bool CapxxxxBlockWrite(TouchCap1293RegAddr_t registerAddress, uint8_t* data, uint8_t length)
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

    esp_res = i2c_master_write_byte(cmd, CAPxxxx_ADDRESS << 1 | I2C_MASTER_WRITE, true);
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

    esp_res = i2c_master_cmd_begin(sI2cPortNumber, cmd, CAPxxxx_WTITE_TIMEOUT_MS / portTICK_RATE_MS);
    if(esp_res != ESP_OK){
        res &= false;
    }

    i2c_cmd_link_delete(cmd);

    return res;
}

static bool CapxxxxForceCalibrateEnabled(void)
{
    Cap1293Reg_t reg = {0};

// cppcheck-suppress unreadVariable
    reg.calibrationActivateAndStatus.cs1Cal = 1;
// cppcheck-suppress unreadVariable
    reg.calibrationActivateAndStatus.cs2Cal = 1;
// cppcheck-suppress unreadVariable
    reg.calibrationActivateAndStatus.cs3Cal = 1;

    return CapxxxxBlockWrite(CALIBRATION_ACTIVATE_AND_STATUS, &reg.byte, 1);
}

static bool CapxxxxSetSensitivity(TouchDriverSensitivity_t sensitivity)
{
    bool res = true;
    Cap1293Reg_t reg = {0};

    res &= CapxxxxBlockRead(SENSITIVITY_CONTROL, &reg.byte, 1);

// cppcheck-suppress unreadVariable
    reg.sensitivityControl.deltaSense = sensitivity;

    res &= CapxxxxBlockWrite(SENSITIVITY_CONTROL, &reg.byte, 1);

    ESP_LOGI(TAG, "sensivity %X", reg.sensitivityControl.deltaSense);

    return res;
}

static bool CapxxxxSetMultiTouchEnabled(void)
{
    bool res = true;
    Cap1293Reg_t reg = {0};

    res &= CapxxxxBlockRead(MULTIPLE_TOUCH_CONFIG, &reg.byte, 1);

// cppcheck-suppress unreadVariable
    reg.mtpConfig.multBlkEn = 0;

    res &= CapxxxxBlockWrite(MULTIPLE_TOUCH_CONFIG, &reg.byte, 1);

    ESP_LOGI(TAG, "multBlkEn %X", reg.byte);

    return res;
}

static bool CapxxxxSetRepeatRateDisabled(void)
{
    bool res = true;
    Cap1293Reg_t reg = {0};

    res &= CapxxxxBlockRead(REPEAT_RATE_ENABLE, &reg.byte, 1);

// cppcheck-suppress unreadVariable
    reg.repeatRateEnable.cs1RptEn = 0;
// cppcheck-suppress unreadVariable
    reg.repeatRateEnable.cs2RptEn = 0;
// cppcheck-suppress unreadVariable
    reg.repeatRateEnable.cs3RptEn = 0;

    res &= CapxxxxBlockWrite(REPEAT_RATE_ENABLE, &reg.byte, 1);
    ESP_LOGI(TAG, "rateDisable %X", reg.byte);

    return res;
}

static bool GpioAlertPinInit(void)
{
    bool res = true;
    gpio_config_t io_conf;

    //interrupt disable
    io_conf.intr_type = GPIO_INTR_DISABLE;

    //bit mask of the pins
    io_conf.pin_bit_mask = (1ULL << CFG_TOUCH_INTERRUPT_PIN);

    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;

    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    
    esp_err_t esp_res = gpio_config(&io_conf);
    if(esp_res != ESP_OK){
        res = false;
    }

    return res;
}

static bool CapxxxxClearIntFlag(void)
{
    bool res = true;
    Cap1293Reg_t reg = {0};

    res &= CapxxxxBlockRead(MAIN_CONTROL, &reg.byte, 1);

// cppcheck-suppress unreadVariable
    reg.mainControl.Int = 0; 

    res &= CapxxxxBlockWrite(MAIN_CONTROL, &reg.byte, 1);

    return res;
}

static bool CapxxxxGetInputStatus(TouchDriverButtonStatus_t* buttoStatus)
{
    bool res = true;
    Cap1293Reg_t reg = {0};

    res &= CapxxxxBlockRead(SENSOR_INPUT_STATUS, &reg.byte, 1);

    ESP_LOGI(TAG, "pressed button %X", reg.byte);
 
    buttoStatus->isPressNow[CFG_TOUCH_BUTTON_NAME_POWER] = reg.sensorInputStatus.cs1; 
    buttoStatus->isPressNow[CFG_TOUCH_BUTTON_NAME_FAN_DEC] = reg.sensorInputStatus.cs2;
    buttoStatus->isPressNow[CFG_TOUCH_BUTTON_NAME_FAN_INC] = reg.sensorInputStatus.cs3;

    return res;
}