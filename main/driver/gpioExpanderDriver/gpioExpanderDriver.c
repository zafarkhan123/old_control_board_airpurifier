/**
 * @file gpioExpanderDriver.c
 *
 * @brief Gpio Expander driver source file
 *
 * @dir gpioExpanderDriver
 * @brief Gpio Expander driver folder
 *
 * @author matfio
 * @date 2021.11.15
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "config.h"

#include <string.h>

#include "driver/i2c.h"
#include "hal/i2c_types.h"

#include "esp_log.h"

#include "gpioExpanderDriver/gpioExpanderDriver.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define PCA9534_GPIO_AS_OUTPUT      (0)
#define PCA9534_GPIO_AS_INPUT       (1)

#define PCA9534_I2C_ADDRESS         (0x3A)
#define I2C_PORT_NUMBER             (CFG_I2C_PORT_NUMBER)

#define PCA9534_WTITE_TIMEOUT_MS    (1000U)
#define PCA9534_READ_TIMEOUT_MS     (1000U)

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS
*****************************************************************************/

typedef enum
{
    INPUT_PORT_REGISTER_ADDRESS         = 0x00,
    OUTPUT_PORT_REGISTER_ADDRESS        = 0x01,
    POLARITY_INVERSION_REGISTER_ADDRESS = 0x02,
    CONFIGURATION_REGISTER_ADDRESS      = 0x03,
}Pca9534RegAddr_t;

typedef union 
{
    GpioExpanderPca9534BitsReg_t bits;
    GpioExpanderPinout_t pinout;
    uint8_t byte;
}Pca9534Reg_t;

/*****************************************************************************
                     PRIVATE VARIABLES
*****************************************************************************/

static i2c_port_t sI2cPortNumber = I2C_PORT_NUMBER;
static bool sExpanderIrqIsSet;
static bool sBuzzerIsOn;
static const char* TAG = "ExpaD";

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief  Read single byte from a expander pca9534
 *  @param registerAddress register address from which we want to read
 *  @param readValue pointer to read data
 *  @return return true if success
 */
static bool Pca9534Read(Pca9534RegAddr_t registerAddress, uint8_t *readValue);

/** @brief  Write single byte to a expander pca9534
 *  @param registerAddress register address from which we want to write
 *  @param writeValue  write data
 *  @return return true if success
 */
static bool Pca9534Write(Pca9534RegAddr_t registerAddress, uint8_t writeValue);

/** @brief Set gpio configuration
 *  @param config configuration bX -> 0 - gpio as output, 1 - gpio as input
 *  @return return true if success
 */
static bool GpioExpanderSetGpioConfiguration(GpioExpanderPinout_t config);

/** @brief  Initialize Interrupt pin as input
 *  @return return true if success
 */
static bool GpioInterruptPinInit(void);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool GpioExpanderDriverInit(void)
{
    bool res = true;
    uint8_t readConf = 0;
    
    // output must be set first or there will be a short sound of buzzer
    Pca9534Reg_t output = {};
    res &= Pca9534Read(OUTPUT_PORT_REGISTER_ADDRESS, &output.byte);
    output.pinout.buzzer = 0;
    output.pinout.ledEnable = 0;
    res &= GpioExpanderDriverSetOutputPort(output.pinout);

    GpioExpanderPinout_t configuration = {
        .wifiSwitch = PCA9534_GPIO_AS_INPUT,
        .limitSwitch3 = PCA9534_GPIO_AS_INPUT,
        .limitSwitch2 = PCA9534_GPIO_AS_INPUT,
        .limitSwitch1 = PCA9534_GPIO_AS_INPUT,

        .ledEnable = PCA9534_GPIO_AS_OUTPUT,
        .buzzer = PCA9534_GPIO_AS_OUTPUT,

        .nc1 = PCA9534_GPIO_AS_INPUT,
        .nc2 = PCA9534_GPIO_AS_INPUT,
    };

    res &= GpioExpanderSetGpioConfiguration(configuration);
    res &= Pca9534Read(CONFIGURATION_REGISTER_ADDRESS, &readConf);

    int cmpRes = memcmp(&configuration, &readConf, sizeof(Pca9534Reg_t));
    if(cmpRes != 0){
        ESP_LOGI(TAG, "incorrect config %X", readConf);
        res = false;
    }

    res &= GpioInterruptPinInit();

    return res;
}

bool GpioExpanderDriverSetOutputPort(GpioExpanderPinout_t outputPort)
{
    Pca9534Reg_t reg = {.pinout = outputPort};
    ESP_LOGI(TAG, "output %X", reg.byte);
    return Pca9534Write(OUTPUT_PORT_REGISTER_ADDRESS, reg.byte);
}

bool GpioExpanderDriverGetInputPort(GpioExpanderPinout_t *inputPort)
{
    Pca9534Reg_t reg = {};
    bool res = Pca9534Read(INPUT_PORT_REGISTER_ADDRESS, &reg.byte);

    ESP_LOGI(TAG, "input %X", reg.byte);
    *inputPort = reg.pinout;

    return res;
}

bool GpioExpanderDriverBuzzerOff(void)
{
    bool res = true;
    Pca9534Reg_t readValue = {};

    res &= Pca9534Read(OUTPUT_PORT_REGISTER_ADDRESS, &readValue.byte);
    // cppcheck-suppress unreadVariable
    readValue.pinout.buzzer = false;
    sBuzzerIsOn = false;
    res &= Pca9534Write(OUTPUT_PORT_REGISTER_ADDRESS, readValue.byte);

    return res;
}

bool GpioExpanderDriverBuzzerOn(void)
{
    bool res = true;
    Pca9534Reg_t readValue = {};

    res &= Pca9534Read(OUTPUT_PORT_REGISTER_ADDRESS, &readValue.byte);
    // cppcheck-suppress unreadVariable
    readValue.pinout.buzzer = true;
    sBuzzerIsOn = true;
    res &= Pca9534Write(OUTPUT_PORT_REGISTER_ADDRESS, readValue.byte);

    return res;
}

bool GpioExpanderDriverIsBuzzerOn(void)
{
    return sBuzzerIsOn;
}

bool GpioExpanderDriverLedOff(void)
{
    bool res = true;
    Pca9534Reg_t readValue = {};

    res &= Pca9534Read(OUTPUT_PORT_REGISTER_ADDRESS, &readValue.byte);
    // cppcheck-suppress unreadVariable
    readValue.pinout.ledEnable = false;
    res &= Pca9534Write(OUTPUT_PORT_REGISTER_ADDRESS, readValue.byte);

    return res;
}

bool GpioExpanderDriverLedOn(void)
{
    bool res = true;
    Pca9534Reg_t readValue = {};

    res &= Pca9534Read(OUTPUT_PORT_REGISTER_ADDRESS, &readValue.byte);
    // cppcheck-suppress unreadVariable
    readValue.pinout.ledEnable = true;
    res &= Pca9534Write(OUTPUT_PORT_REGISTER_ADDRESS, readValue.byte);

    return res;
}

bool GpioExpanderDriverIsInterruptSet(void)
{
    return sExpanderIrqIsSet;
}

void GpioExpanderDriverIrqChangeCallback(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;

    if(gpio_num == CFG_GPIO_EXPANDER_INT_GPIO_PIN)
    {
        sExpanderIrqIsSet = true;
    }
}

void GpioExpanderDriverClearIrq(void)
{
    sExpanderIrqIsSet = false;
}

void GpioExpanderDriverPrintInputStatus(GpioExpanderPinout_t inputPort)
{
    ESP_LOGI(TAG, "wifi switch %s", (inputPort.wifiSwitch) ? "On" : "Off");
    ESP_LOGI(TAG, "limit 1 switch %s", (inputPort.limitSwitch1) ? "Open" : "Close");
    ESP_LOGI(TAG, "limit 2 switch %s", (inputPort.limitSwitch2) ? "Open" : "Close");
    ESP_LOGI(TAG, "limit 3 switch %s", (inputPort.limitSwitch3) ? "Open" : "Close");
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool Pca9534Read(Pca9534RegAddr_t registerAddress, uint8_t *readValue)
{
    if(readValue == NULL){
        return false;
    }

    switch(registerAddress){
        case INPUT_PORT_REGISTER_ADDRESS:
        case OUTPUT_PORT_REGISTER_ADDRESS:
        case POLARITY_INVERSION_REGISTER_ADDRESS:
        case CONFIGURATION_REGISTER_ADDRESS:
        {
            break;
        }
        default:
        {
            return false;
        }
    }

    bool res = true;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    /*
     * ______________________________________________________
     * | start | slave_addr + wr_bit + ack | command_byte + ack  |
     * --------|---------------------------|-----------------|
    */
    esp_err_t esp_res = i2c_master_start(cmd);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write_byte(cmd, PCA9534_I2C_ADDRESS << 1 | I2C_MASTER_WRITE, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write_byte(cmd, registerAddress, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    /*
     * ______________________________________________________________
     * | start | slave_addr + rd_bit + ack | reg_data + nack | stop |
     * --------|---------------------------|-----------------|------|
    */
    esp_res = i2c_master_start(cmd);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write_byte(cmd, PCA9534_I2C_ADDRESS << 1 | I2C_MASTER_READ, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_read(cmd, readValue, 1, I2C_MASTER_LAST_NACK);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_stop(cmd);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_cmd_begin(sI2cPortNumber, cmd, PCA9534_READ_TIMEOUT_MS / portTICK_RATE_MS);
    if(esp_res != ESP_OK){
        res &= false;
    }

    i2c_cmd_link_delete(cmd);

    return res;
}

static bool Pca9534Write(Pca9534RegAddr_t registerAddress, uint8_t writeValue)
{
    switch(registerAddress){
        case INPUT_PORT_REGISTER_ADDRESS:
        case OUTPUT_PORT_REGISTER_ADDRESS:
        case POLARITY_INVERSION_REGISTER_ADDRESS:
        case CONFIGURATION_REGISTER_ADDRESS:
        {
            break;
        }
        default:
        {
            return false;
        }
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

    esp_res = i2c_master_write_byte(cmd, PCA9534_I2C_ADDRESS << 1 | I2C_MASTER_WRITE, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write_byte(cmd, registerAddress, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_write(cmd, &writeValue, 1, true);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_stop(cmd);
    if(esp_res != ESP_OK){
        res &= false;
    }

    esp_res = i2c_master_cmd_begin(sI2cPortNumber, cmd, PCA9534_WTITE_TIMEOUT_MS / portTICK_RATE_MS);
    if(esp_res != ESP_OK){
        res &= false;
    }

    i2c_cmd_link_delete(cmd);

    return res;
}

static bool GpioExpanderSetGpioConfiguration(GpioExpanderPinout_t config)
{
    Pca9534Reg_t reg = {.pinout = config};
    ESP_LOGI(TAG, "config %X", reg.byte);

    return Pca9534Write(CONFIGURATION_REGISTER_ADDRESS, reg.byte);
}

static bool GpioInterruptPinInit(void)
{
    bool res = true;
    gpio_config_t io_conf;

    //interrupt disable
    io_conf.intr_type = GPIO_INTR_NEGEDGE;

    //bit mask of the pins
    io_conf.pin_bit_mask = (1ULL << CFG_GPIO_EXPANDER_INT_GPIO_PIN);

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

    //read pins level first time.
    if(gpio_get_level(CFG_GPIO_EXPANDER_INT_GPIO_PIN) == false){
         sExpanderIrqIsSet = true;
    }

    return res;
}