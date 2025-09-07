/**
 * @file adcDriver.c
 *
 * @brief Adc driver source file
 *
 * @dir adcDriver
 * @brief Adc driver folder
 *
 * @author matfio
 * @date 2021.08.24
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "adcDriver.h"

#include "config.h"

#include <esp_log.h>

#include "driver/adc.h"
#include "esp_adc_cal.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define ADC_DEFAULT_VREF (1100U)

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

typedef struct
{
    const adc_channel_t channel;
    esp_adc_cal_characteristics_t characteristic;
}AdcDriverSetting_t;

static AdcDriverSetting_t sAdcSetting[ADC_DRIVER_CHANNEL_COUNT] = {
    [ADC_DRIVER_CHANNEL_UV_1] = {.channel = ADC_CHANNEL_6}, //CFG_UV_LAMP_BALLAST_1_FAULT_ADC_PIN (34U)
    [ADC_DRIVER_CHANNEL_UV_2] = {.channel = ADC_CHANNEL_7}, //CFG_UV_LAMP_BALLAST_2_FAULT_ADC_PIN (35U)
};

static const char *TAG = "adc";

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool AdcDriverInit(void)
{
    const adc_atten_t atten = ADC_ATTEN_DB_11;
    const adc_bits_width_t width = ADC_WIDTH_BIT_12;

    //Check if TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Two Point: Supported");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Vref: Supported");
    }

    adc1_config_width(width);

    for(uint16_t idx = 0; idx < ADC_DRIVER_CHANNEL_COUNT; ++idx){
        adc1_config_channel_atten(sAdcSetting[idx].channel, atten);

        esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, atten, width, ADC_DEFAULT_VREF, &sAdcSetting[idx].characteristic);

        if(val_type == ESP_ADC_CAL_VAL_EFUSE_TP){
            ESP_LOGI(TAG, "%d characterized using Two Point Value", idx);
        }else if(val_type == ESP_ADC_CAL_VAL_EFUSE_VREF){
            ESP_LOGI(TAG, "%d characterized using eFuse Vref", idx);
        }else{
            ESP_LOGI(TAG, "%d characterized using Default Vref", idx);
        }
    }
    ESP_LOGI(TAG, "");

    return true;
}

uint32_t AdcDriverGetRawData(AdcDriverChannel_t adcChannel)
{
    if(adcChannel >= ADC_DRIVER_CHANNEL_COUNT){
        ESP_LOGW(TAG, "incorrect channel %d", adcChannel);
        return 0;
    }

    return adc1_get_raw(sAdcSetting[adcChannel].channel);
}

float AdcDriverGetMilliVoltageData(AdcDriverChannel_t adcChannel)
{
    if(adcChannel >= ADC_DRIVER_CHANNEL_COUNT){
        ESP_LOGW(TAG, "incorrect channel %d", adcChannel);
        return 0;
    }

    uint32_t adcRaw = adc1_get_raw(sAdcSetting[adcChannel].channel);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adcRaw, &sAdcSetting[adcChannel].characteristic);

    float res = voltage * 2.0f; // at the input there is a voltage divider by 2
    return res;
}