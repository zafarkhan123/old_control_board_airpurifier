/**
 * @file config.h
 *
 * @brief configuration header file
 *
 * @author matfio
 * @date 2021.08.23
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

/*** Build ***********************************************************************/
#define CFG_BUILD_RELEASE           (1)
#define CFG_BUILD_DEBUG             (2)
#define CFG_BUILD_PRODUCTION        (3)

#ifndef CFG_BUILD_TYPE
#endif

/*** Device *********************************************************************/

#define CFG_DEFAULT_DEVICE_NAME ("ICoNPro")

/*** FtTool *********************************************************************/
#define CFG_FT_TOOL_ENABLE (0U)
#define CFG_FT_TOOL_RXD_GPIO_PIN (13U)      // TCK
#define CFG_FT_TOOL_TXD_GPIO_PIN (14U)      // TMS

/*** Partition *******************************************************************/
/*
    If set to 0 you need to flash factory partition
    Look to 10655tco21-feasibility-icon-esp32\factory_settings\README.md
*/
#define CFG_FACTORY_PARTITION_DISABLE (0U)

/*** Hepa ************************************************************************/

#define CFG_HEPA_SERVICE_LIFETIME_HOURS (20U * 1000U)
#define CFG_HEPA_SERVICE_REPLACEMENT_REMINDER (18U * 1000U)

/*** UV Lamp ********************************************************************/
#define CFG_UV_LAMP_BALLAST_1_ON_OFF_GPIO_PIN (22U)     // REL1
#define CFG_UV_LAMP_BALLAST_2_ON_OFF_GPIO_PIN (19U)     // REL2

#define CFG_UV_LAMP_BALAST_OFF_MIN_VOLT_LEVEL (1800)
#define CFG_UV_LAMP_BALAST_OFF_MAX_VOLT_LEVEL (2200)
#define CFG_UV_LAMP_BALAST_ON_MIN_VOLT_LEVEL  (2700)
#define CFG_UV_LAMP_BALAST_ON_MAX_VOLT_LEVEL  (3100)

#define CFG_UV_LAMP_BALLAST_1_FAULT_ADC_PIN (34U)       // UV1_ADC
#define CFG_UV_LAMP_BALLAST_2_FAULT_ADC_PIN (35U)       // UV2_ADC

#define CFG_UV_LAMP_SERVICE_LIFETIME_HOURS (10U * 1000U)
#define CFG_UV_LAMP_SERVICE_REPLACEMENT_REMINDER (9U * 1000U)

#define CFG_UV_LAMP_ECO_MODE_SWITCH_TIMIE_SEC (30U * 60U)

#define CFG_UV_LAMP_ON_DELAY_TIMIE_SEC (5U)

/*** RGB Led ********************************************************************/
#define CFG_RGB_LED_DATA_GPIO_PIN (17U)                 // SLED_DATA

/*** Fan ***********************************************************************/
#define CFG_FAN_PWM_GPIO_PIN (23U)                      // FAN_PWM
#define CFG_FAN_TACHO_GPIO_PIN (4U)                     // FAN_TACHO

/*** Common I2c Communication **************************************************/
#define CFG_I2C_PORT_NUMBER (0)

#define CFG_I2C_DATA_PIN (26U)                          // I2C_SDA
#define CFG_I2C_CLK_PIN (27U)                           // I2C_SCL

#define CFG_I2C_FREQ_HZ (100U * 1000U)

/*** Touch *********************************************************************/
#define CFG_TOUCH_INTERRUPT_PIN (36U)                   // TP_INT

typedef enum{
    CFG_TOUCH_BUTTON_NAME_POWER = 0,
    CFG_TOUCH_BUTTON_NAME_FAN_INC,
    CFG_TOUCH_BUTTON_NAME_FAN_DEC,
    CFG_TOUCH_BUTTON_NAME_COUNT
}CfgTouchButtonName_t;

#define CFG_TOUCH_DEFAULT_LOGO_COLOR (0x00ffffff) // 0x00 - irrelevant, ff - r, ff - g, ff - b

/*** Gpio expander ************************************************************/
#define CFG_GPIO_EXPANDER_INT_GPIO_PIN (16U)            // EXP_INT

/*** Common SPI Communication ************************************************/
#define CFG_SPI_HOST_NUMBER (1U)
#define CFG_SPI_CLOCK_MHZ (6U) 

#define CFG_SPI_SCLK_GPIO  (25U)                        // SPI_SCL
#define CFG_SPI_MOSI_GPIO  (33U)                        // SPI_MOSI
#define CFG_SPI_MISO_GPIO  (32U)                        // SPI_MISO

/*** Ethernet **************************************************************/
#define CFG_ETHERNET_CS_GPIO    (18U)                   // ETH_CS
#define CFG_ETHERNET_INT_GPIO   (39U)                   // ETH_INT
#define CFG_ETHERNET_RST_GPIO   (21U)                   // ETH_RST

/*** External Flash *********************************************************/
#define CFG_EXTERNAL_FLASH_CS_GPIO     (5U)             // FLASH_CS
#define CFG_EXTERNAL_FLASH_RST_GPIO    (0U)             // FLASH_RST

/*** HTTP Client **************************************************************/
#define CFG_HTTP_CLIENT_ENABLE (1U)
#define CFG_HTTP_CLIENT_PORT_NUMBER (443U)
#define CFG_HTTP_CLIENT_NO_INTERNET_ACCESS_NUMBER_OF_SAVED_POST (16U)

/*** Wifi **************************************************************/
#define CFG_WIFI_DEFAULT_AP_SSID (CFG_DEFAULT_DEVICE_NAME)
#define CFG_WIFI_AP_SSID_STRING_LEN (32U) // idf limitation (do not change)
#define CFG_WIFI_AP_PASS ("ICON126C#30376")
#define CFG_WIFI_AP_CONN (2U)

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/
