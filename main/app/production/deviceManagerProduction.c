/**
 * @file deviceManagerProduction.c
 *
 * @brief Device manager production source file
 *
 * @dir production
 * @brief production folder
 *
 * @author matfio
 * @date 2022.01.26
 * @version v1.0
 *
 * @copyright 2022 Fideltronik R&D - all rights reserved.
 */

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "driver/uart.h"

#include "gpioExpanderDriver/gpioExpanderDriver.h"
#include "rtcDriver/rtcDriver.h"
#include "externalFlashDriver/externalFlashDriver.h"
#include "ethernetDriver/ethernetDriver.h"
#include "ledDriver/ledDriver.h"
#include "uvLampDriver/uvLampDriver.h"
#include "adcDriver/adcDriver.h"
#include "timeDriver/timeDriver.h"

#include "wifi/wifi.h"
#include "touch/touch.h"
#include "fan/fan.h"

#include "cloud/iotHubClient.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define DEVICEMANAGER_TASK_DELAY_MS (100U)

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

#define ANALOG_IMPUT_MIN_MILI_VOLTAGE (3000.0f)
#define ANALOG_IMPUT_MAX_MILI_VOLTAGE (3600.0f)

#define TEST_PASS_STRING "PASS"
#define TEST_FAIL_STRING "FAIL"

static const char* TAG = "devManProd";

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Run rtc test
 *  @return return true if success
 */
static bool ExternalRtcTest(void);

/** @brief Run led test
 *  @return return true if success
 */
static bool LedTest(void);

/** @brief Wait to press 't' in keybord
 */
static void WaitingToYes(void);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

void DeviceManagerProductionMainLoop(void *argument)
{
    esp_log_level_set(TAG, ESP_LOG_ERROR);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // setting needed to scanf
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    ESP_LOGI(TAG, "silicon revision %d, ", chip_info.revision);

    ESP_LOGI(TAG, "%dMB %s flash", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    ESP_LOGI(TAG, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    ESP_LOGI(TAG, "                      TEST URZADZENIA POCZATEK ");
    ESP_LOGI(TAG, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

    ESP_LOGI(TAG, "Test wiatraka start");
    SettingDevice_t settingDevice = {.restore = {.deviceStatus = {.isDeviceOn = true, .fanLevel = 2}}};
    bool res = true;
    res &= FanInit();
    res &= FanLevelChange(&settingDevice);
    vTaskDelay(1U * 1000U);
    ESP_LOGI(TAG, "Status ustawienia pwm %d", res);

    ESP_LOGI(TAG, "Test komunikacji z expanderem gpio");
    res = GpioExpanderDriverInit();
    if(res == true){
        ESP_LOGI(TAG, "Wynik testu expandera gpio \t\t\t\t%s", TEST_PASS_STRING);
    }else{
        ESP_LOGE(TAG, "Wynik testu expandera gpio \t\t\t\t%s", TEST_FAIL_STRING);
    }
    vTaskDelay(DEVICEMANAGER_TASK_DELAY_MS);

    ESP_LOGI(TAG, "Test komunikacji z zewnetrznym rtc");
    res = ExternalRtcTest();
    if(res == true){
        ESP_LOGI(TAG, "Wynik testu rtc \t\t\t\t\t\t%s", TEST_PASS_STRING);
    }else{
        ESP_LOGE(TAG, "Wynik testu rtc \t\t\t\t\t\t%s", TEST_FAIL_STRING);
    }
    vTaskDelay(DEVICEMANAGER_TASK_DELAY_MS);

    ESP_LOGI(TAG, "Test dodatkowej płytki PCB z ukladem ethernet");
    ESP_LOGI(TAG, "Dodatkowe PCB musi byc wpiete w odpowiednie miejsce");
    res = ethernetDriverInit();
    res &= ethernetDriverIsAdditionalPcbConnected();
    if(res == true){
        ESP_LOGI(TAG, "Wynik testu płytki PCB ethernet \t\t\t\t%s", TEST_PASS_STRING);
    }else{
        ESP_LOGE(TAG, "Wynik testu płytki PCB ethernet \t\t\t\t%s", TEST_FAIL_STRING);
    }
    vTaskDelay(DEVICEMANAGER_TASK_DELAY_MS);

    ESP_LOGI(TAG, "Test komunikacji z układem zewnętrznego flash");
    res = ExternalFlashDriverInit();
    if(res == true){
        ESP_LOGI(TAG, "Wynik testu komunikacji z flash \t\t\t\t%s", TEST_PASS_STRING);
    }else{
        ESP_LOGE(TAG, "Wynik testu komunikacji z flash \t\t\t\t%s", TEST_FAIL_STRING);
    }
    vTaskDelay(DEVICEMANAGER_TASK_DELAY_MS);

    ESP_LOGI(TAG, "Test LED-ow");
    ESP_LOGI(TAG, "Wszystkie 16 Led-ow zapali sie na kolor bialy");
    ESP_LOGI(TAG, "Zapalily sie wszystkie led-y?");
    LedTest();
    vTaskDelay(DEVICEMANAGER_TASK_DELAY_MS);
    WaitingToYes();
    ESP_LOGI(TAG, "Wynik testow LED-ow \t\t\t\t\t%s", TEST_PASS_STRING);
    GpioExpanderDriverLedOff();
    vTaskDelay(DEVICEMANAGER_TASK_DELAY_MS);

    ESP_LOGI(TAG, "Test dzialania brzeczka");
    GpioExpanderDriverBuzzerOn();
    vTaskDelay(1U * 1000U);
    GpioExpanderDriverBuzzerOff();
    ESP_LOGI(TAG, "Slyszany byl dzwiek?");
    WaitingToYes();
    ESP_LOGI(TAG, "Wynik testow brzeczka \t\t\t\t\t%s", TEST_PASS_STRING);
    vTaskDelay(DEVICEMANAGER_TASK_DELAY_MS);

    ESP_LOGI(TAG, "Test komunikacji z układem dotyku");
    res = TouchInit();
    if(res == true){
        ESP_LOGI(TAG, "Wynik testu komunikacji z układem dotyku \t\t\t%s", TEST_PASS_STRING);
    }else{
        ESP_LOGE(TAG, "Wynik testu komunikacji z układem dotyku \t\t\t%s", TEST_FAIL_STRING);
    }
    vTaskDelay(DEVICEMANAGER_TASK_DELAY_MS);

    int16_t revolutions = 0;
    FanGetTachoRevolutionsPerSecond(&revolutions);
    if(revolutions > 10){
        ESP_LOGI(TAG, "Wynik testu PWM i impulsatora wiatraka \t\t\t%s", TEST_PASS_STRING);
    }else{
        ESP_LOGE(TAG, "Wynik testu PWM i impulsatora wiatraka \t\t\t%s", TEST_FAIL_STRING);
    }

    ESP_LOGI(TAG, "Test dzialania przekaznikow");
    ESP_LOGI(TAG, "Oby dwa przekażnik sa wylaczone?");
    WaitingToYes();
    UvLampDriverInit();
    ESP_LOGI(TAG, "Przekażnik 1 jest wlaczony, przekaznik 2 jest wylaczony");
    UvLampDriverSetLevel(UV_LAMP_1, 1);
    UvLampDriverSetLevel(UV_LAMP_2, 0);
    ESP_LOGI(TAG, "Czy tak jest?");
    WaitingToYes();
    ESP_LOGI(TAG, "Przekażnik 1 jest wylaczony, przekaznik 2 jest wlaczony");
    UvLampDriverSetLevel(UV_LAMP_1, 0);
    UvLampDriverSetLevel(UV_LAMP_2, 1);
    ESP_LOGI(TAG, "Czy tak jest?");
    WaitingToYes();
    ESP_LOGI(TAG, "Obydwa przekazniki sa wlaczone?");
    UvLampDriverSetLevel(UV_LAMP_1, 1);
    UvLampDriverSetLevel(UV_LAMP_2, 1);
    WaitingToYes();
    ESP_LOGI(TAG, "Wynik testu przekaznikow \t\t\t\t\t%s", TEST_PASS_STRING);

    ESP_LOGI(TAG, "Test dzialania wifi");
    ESP_LOGI(TAG, "Czy jest widaina siec wifi o nazwie \"ICON_PRO_2022.01.000X\" (gdzie X jest od 1 do 4) ?");
    WaitingToYes();
    ESP_LOGI(TAG, "Wynik testu wifi \t\t\t\t\t\t%s", TEST_PASS_STRING);

    bool butonsPressTestRun = true;
    bool butonSw1PressTestRun = true;
    bool butonSw2PressTestRun = false;
    bool butonSw3PressTestRun = false;
    TouchButtons_t buttons = {};
    ESP_LOGI(TAG, "Test dzialania PCB dotykowego");
    ESP_LOGI(TAG, "Nacisnij przycisk SW1 przez okolo 4 sekundy");

    bool switchTestRun = false;
    bool switch1TestRun = false;
    bool switch2TestRun = false;
    bool switch3TestRun = false;
    bool switch4TestRun = false;
    GpioExpanderPinout_t inputPort = {};

    bool analogInputTestRun = false;
    bool analogInput1TestRun = false;
    bool analogInput2TestRun = false;
    
    for(;;)
    {
        if(butonsPressTestRun == true){
            bool buttonPress = TouchButtonStatus(&buttons);
            if(buttonPress == true){
                if(butonSw1PressTestRun == true){
                    if((buttons.status[CFG_TOUCH_BUTTON_NAME_POWER] == TOUCH_BUTTON_PRESS_LONG) || (buttons.status[CFG_TOUCH_BUTTON_NAME_POWER] == TOUCH_BUTTON_PRESS_VERY_LONG)){
                        ESP_LOGI(TAG, "Wykryto dotyk na przycisku SW1");
                        butonSw1PressTestRun = false;
                        butonSw2PressTestRun = true;
                        ESP_LOGI(TAG, "Nacisnij przycisk SW2 przez okolo 4 sekundy");
                    }
                }

                if(butonSw2PressTestRun == true){
                    if((buttons.status[CFG_TOUCH_BUTTON_NAME_FAN_DEC] == TOUCH_BUTTON_PRESS_LONG) || (buttons.status[CFG_TOUCH_BUTTON_NAME_FAN_DEC] == TOUCH_BUTTON_PRESS_VERY_LONG)){
                        ESP_LOGI(TAG, "Wykryto dotyk na przycisku SW2");
                        butonSw2PressTestRun = false;
                        butonSw3PressTestRun = true;
                        ESP_LOGI(TAG, "Nacisnij przycisk SW3 przez okolo 4 sekundy");
                    }
                }

                if(butonSw3PressTestRun == true){
                    if((buttons.status[CFG_TOUCH_BUTTON_NAME_FAN_INC] == TOUCH_BUTTON_PRESS_SHORT) || (buttons.status[CFG_TOUCH_BUTTON_NAME_FAN_INC] == TOUCH_BUTTON_PRESS_LONG) || (buttons.status[CFG_TOUCH_BUTTON_NAME_FAN_INC] == TOUCH_BUTTON_PRESS_VERY_LONG)){
                        ESP_LOGI(TAG, "Wykryto dotyk na przycisku SW3");
                        butonSw3PressTestRun = false;
                        butonsPressTestRun = false;
                        ESP_LOGI(TAG, "Wynik testu PCB dotykowego \t\t\t\t%s", TEST_PASS_STRING);

                        switchTestRun = true;
                        switch1TestRun = true;
                        ESP_LOGI(TAG, "Test dzialania wejsc");
                        ESP_LOGI(TAG, "Naciśnij przycisk dla wejscia numer 1");
                    }
                }
            }
        }

        if(switchTestRun == true){
            if(GpioExpanderDriverIsInterruptSet()){
                GpioExpanderDriverGetInputPort(&inputPort);
                GpioExpanderDriverClearIrq();

                if(switch1TestRun == true){
                    if((inputPort.wifiSwitch == false) && (inputPort.limitSwitch3 == true) && (inputPort.limitSwitch2 == true) && (inputPort.limitSwitch1 == true)){
                        ESP_LOGI(TAG, "Wykryto wcisniecie przycisku dla wejscia 1");
                        switch1TestRun = false;
                        switch2TestRun = true;
                        ESP_LOGI(TAG, "Naciśnij przycisk dla wejscia numer 2");
                    }
                }

                if(switch2TestRun == true){
                    if((inputPort.wifiSwitch == true) && (inputPort.limitSwitch3 == false) && (inputPort.limitSwitch2 == true) && (inputPort.limitSwitch1 == true)){
                        ESP_LOGI(TAG, "Wykryto wcisniecie przycisku dla wejscia 2");
                        switch2TestRun = false;
                        switch3TestRun = true;
                        ESP_LOGI(TAG, "Naciśnij przycisk dla wejscia numer 3");
                    }
                }

                if(switch3TestRun == true){
                    if((inputPort.wifiSwitch == true) && (inputPort.limitSwitch3 == true) && (inputPort.limitSwitch2 == false) && (inputPort.limitSwitch1 == true)){
                        ESP_LOGI(TAG, "Wykryto wcisniecie przycisku dla wejscia 3");
                        switch3TestRun = false;
                        switch4TestRun = true;
                        ESP_LOGI(TAG, "Naciśnij przycisk dla wejscia numer 4");
                    }
                }

                if(switch4TestRun == true){
                    if((inputPort.wifiSwitch == true) && (inputPort.limitSwitch3 == true) && (inputPort.limitSwitch2 == true) && (inputPort.limitSwitch1 == false)){
                        ESP_LOGI(TAG, "Wykryto wcisniecie przycisku dla wejscia 4");
                        switch4TestRun = false;
                        switchTestRun = false;
                        ESP_LOGI(TAG, "Wynik testu wejsc \t\t\t\t\t%s", TEST_PASS_STRING);

                        analogInputTestRun = true;
                        analogInput1TestRun = true;

                        ESP_LOGI(TAG, "Test wejsc analogowych");
                        ESP_LOGI(TAG, "Przyloz napięcie 3.3 V do wejścia analogowego 1");
                    }
                }
            }
        }

        if(analogInputTestRun == true){
            if(analogInput1TestRun == true){
                float uvVolt = AdcDriverGetMilliVoltageData(ADC_DRIVER_CHANNEL_UV_1);
                if((uvVolt > ANALOG_IMPUT_MIN_MILI_VOLTAGE) && ((uvVolt < ANALOG_IMPUT_MAX_MILI_VOLTAGE))){
                    ESP_LOGI(TAG, "Wykryto napiecie 3.3 [V] (%.2f[mV]) na wejsciu analogowym 1", uvVolt);
                    analogInput1TestRun = false;

                    analogInput2TestRun = true;
                    ESP_LOGI(TAG, "Przyloz napięcie 3.3 V do wejścia analogowego 2");
                }
            }

            if(analogInput2TestRun == true){
                float uvVolt = AdcDriverGetMilliVoltageData(ADC_DRIVER_CHANNEL_UV_2);
                if((uvVolt > ANALOG_IMPUT_MIN_MILI_VOLTAGE) && ((uvVolt < ANALOG_IMPUT_MAX_MILI_VOLTAGE))){
                    ESP_LOGI(TAG, "Wykryto napiecie 3.3 [V] (%.2f[mV]) na wejsciu analogowym 2", uvVolt);
                    analogInput2TestRun = false;

                    analogInputTestRun = false;
                    ESP_LOGI(TAG, "Wynik testu wejsc analogowych \t\t\t\t%s", TEST_PASS_STRING);

                    ESP_LOGI(TAG, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
                    ESP_LOGI(TAG, "                      TEST URZADZENIA KONIEC ");
                    ESP_LOGI(TAG, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

                    float testTime = TimeDriverGetSystemTickMs();
                    testTime = testTime / 1000.0f;
                    ESP_LOGI(TAG, "Test dla tego urzadzenia trwal %.2f [sekundy]", testTime);

                    ESP_LOGI(TAG, "");
                    ESP_LOGI(TAG, "Gdyby pojawil sie jakis napis w kolerze czerwonym to wystapil blad");
                    ESP_LOGI(TAG, "Przejrzyj jeszcze raz logi");
                }
            }
        }

        vTaskDelay(DEVICEMANAGER_TASK_DELAY_MS);
    }
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static bool ExternalRtcTest(void)
{
    bool res = true;

    struct tm setTime = {.tm_year = 122, .tm_mon = 1, .tm_mday = 10, .tm_hour = 11, .tm_min = 30, .tm_sec = 30};
    struct tm getTime = {};

    res &= RtcDriverSetDateTime(&setTime);
    vTaskDelay(6U * 1000U);
    res &= RtcDriverGetDateTime(&getTime);

    if(res == false){
        return false;
    }

    if((setTime.tm_year != getTime.tm_year) || (setTime.tm_mon != getTime.tm_mon) || (setTime.tm_mday != getTime.tm_mday) || (setTime.tm_hour != getTime.tm_hour) || (setTime.tm_min != getTime.tm_min)){
        res = false;
    }

    if((setTime.tm_sec + 5) > getTime.tm_sec){
        res = false;
        ESP_LOGE(TAG, "there is something wrong with the quartz");   
    }

    return res;
}

static bool LedTest(void)
{
    bool res = true;

    res &= LedDriverInit();
    for(uint16_t idx = 0; idx < LED_NAME_COUNT; ++idx)
    {
        res &= LedDriverSetColor(idx, LED_COLOR_WHITE);
    }

    res &= LedDriverChangeColor();

    return res;
}

static void WaitingToYes(void)
{
    ESP_LOGI(TAG, "Wciśnij \"t\" gdy tak");

    while(true){
        if(getchar() == 't'){
            break;
        }
        vTaskDelay(DEVICEMANAGER_TASK_DELAY_MS);
    }
}