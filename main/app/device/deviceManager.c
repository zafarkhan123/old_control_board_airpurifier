/**
 * @file deviceManager.c
 *
 * @brief Device manager source file
 *
 * @dir device
 * @brief Device folder
 *
 * @author matfio
 * @date 2021.08.30
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "config.h"
#include "setting.h"

#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_ota_ops.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "alarmHandling.h"
#include "cloud/iotHubClient.h"
#include "webServer/webServer.h"

#include "fan/fan.h"
#include "led/led.h"
#include "touch/touch.h"
#include "scheduler/scheduler.h"
#include "uvLamp/uvLamp.h"
#include "wifi/wifi.h"
#include "ota/ota.h"
#include "location/location.h"

#include "timeDriver/timeDriver.h"
#include "adcDriver/adcDriver.h"
#include "timerDriver/timerDriver.h"
#include "factorySettingsDriver/factorySettingsDriver.h"
#include "mcuDriver/mcuDriver.h"
#include "rtcDriver/rtcDriver.h"
#include "ethernetDriver/ethernetDriver.h"
#include "gpioExpanderDriver/gpioExpanderDriver.h"

#include <sys/time.h>
#include "esp_timer.h"
#include "test.h"

/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define DEVICEMANAGER_TASK_DELAY_MS (100U)
#define DEVICEMANAGER_UPDATE_STATUS_MS (60U * 1000U)                                                        // 1 minute
#define DEVICEMANAGER_UPDATE_TIMERS_MS (60U * 1000U)                                                        // 1 minute
#define DEVICEMANAGER_SAVE_SETTING_MS (10U * 60U * 1000U)                                                   // 10 minute
#define DEVICEMANAGER_NEW_FIRMWARE_VERYFICATION_TIMEOUT_MS (60U * 1000U)                                    // 1 minute
#define DEVICEMANAGER_WIFI_CONNECTION_TRY_INTERVAL_MS (5U * 1000U)                                          // 5 secunds
#define DEVICEMANAGER_WIFI_CONNECTION_TRY_NEW_AP_MS (15U * 1000U)                                           // 15 secunds
#define DEVICEMANAGER_FACTORY_RESTART_TIMEOUT_MS (10U * 1000U)                                              // 10 secunds

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static const char* TAG = "devMan";

/*****************************************************************************
                         PRIVATE FUNCTION DECLARATION
*****************************************************************************/

/** @brief Print basic info
 */
static void PrintDeviceStatusInfo(void);

/** @brief Print restart reason
 */
static void PrintRestartReason(void);

/** @brief Print status
 */
static void PrintStatus(void);

/** @brief Print setting
 */
static void PrintSetting(SettingDevice_t* setting);

/** @brief Refactor default setting
 * Restore factory scheduler, ....
 */
static void RestoreFactoryDeviceSetting(SettingDevice_t* setting);

/** @brief Device restart
 */
static void DeviceRestart(SettingDevice_t* setting);

/** @brief Whether the settings have changed. There may be a change fan speed of the led color
 *  @param newSetting [in] pointer to SettingDevice_t
 *  @param oldSetting [in] pointer to SettingDevice_t
 *  @return return true if yes
 */
static bool IsSettingChange(const SettingDevice_t* newSetting, const SettingDevice_t* oldSetting);

/** @brief The timer values stored in nvs need ​​to changed
 *  @param newSetting [in] pointer to SettingDevice_t
 *  @param oldSetting [in] pointer to SettingDevice_t
 *  @return return true if yes
 */
static bool TimersChanges(const SettingDevice_t* newSetting, const SettingDevice_t* oldSetting);

/** @brief Need to save the settings to nvs. To don't lose after a power off.
 *  @param newSetting [in] pointer to SettingDevice_t
 *  @param oldSetting [in] pointer to SettingDevice_t
 *  @return return true if yes
 */
static bool NeedSaveSettingToNvm(const SettingDevice_t* newSetting, const SettingDevice_t* oldSetting);

/** @brief Turn on/off wifi depending of wifi switch
 *  @param setting [in] pointer to SettingDevice_t
 *  @param inputPort [in] pointer to GpioExpanderPinout_t
 *  @return return true if wifi on
 */
static bool WifiButtonOperation(SettingDevice_t* setting, const GpioExpanderPinout_t *inputPort);

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

void DeviceManagerMainLoop(void *argument)
{
    ESP_LOGI(TAG, "DeviceManagerMainLoop start");

    bool runFirstTime = false;
    bool isFactoryResetButtonsPressContinuously = false;
    bool retryConnectToWifi = true;
    
    bool isVeryficationNeeded = OtaIsVeryficationNeed();

    vTaskDelay(1000U);
    LedInit();
    ESP_LOGI(TAG, "Led RGB Init");
    vTaskDelay(100U);

    int64_t updtateStatusTime = 0;
    int64_t updtateTimerTime = 0;
    int64_t saveSettingTime = 0;
    int64_t newFirmwareTime = 0;
    int64_t wifiConnectionTryTime = 0;
    int64_t factorySequenceStartTime = 0;

    SettingDevice_t deviceSetting = {};
    SettingDevice_t deviceSettingOld = {};
    SettingDevice_t deviceSettingNvs = {};
    TouchButtons_t buttons = {};
    GpioExpanderPinout_t inputPort = {};

    SettingGet(&deviceSetting);
    SettingGet(&deviceSettingNvs);
    TimerDriverSetTimers(&deviceSetting);
    AlarmHandlingTimersWornOutCheck(&deviceSetting);

    deviceSetting.ethernetPcbAdded = ethernetDriverIsAdditionalPcbConnected();
    deviceSetting.newFirmwareVeryfication = isVeryficationNeeded;
    SettingSet(&deviceSetting);
    
    PrintDeviceStatusInfo();
    PrintRestartReason();

    TestInit();

    ESP_LOGI(TAG, "is veryfication needed %s", (isVeryficationNeeded) ? "YES" : "NO");
    ESP_LOGI(TAG, "set time according to external rtc");

    // read time from external rtc and set time in esp32 device
    bool rtcError = RtcDriverIsError();
    ESP_LOGI(TAG, "rtc error occur %s", (rtcError) ? "YES" : "NO");
    if(rtcError == false){
        struct tm rtcTime = {};
        char rtcTimeStr[32] = {};

        RtcDriverGetDateTime(&rtcTime);
        TimeDriverSetEspTime(&rtcTime);

        strftime(rtcTimeStr, 32 ,"%F %X", &rtcTime);
        ESP_LOGI(TAG, "unix time from rtc %s", rtcTimeStr);
        
        float offset = LocationGetUtcOffset();
        ESP_LOGI(TAG, "offset to GMT time %.2f", offset);

        ESP_LOGI(TAG, "update esp32 time");
        ESP_LOGI(TAG, "local %s", TimeDriverGetLocalTimeStr());
    }

    PrintSetting(&deviceSetting);

    ESP_LOGI(TAG, "read first time gpio expander");
    GpioExpanderDriverGetInputPort(&inputPort);
    GpioExpanderDriverPrintInputStatus(inputPort);

    factorySequenceStartTime = TimeDriverGetSystemTickMs();
    if((inputPort.wifiSwitch == false) && (inputPort.limitSwitch3 == true)){
        isFactoryResetButtonsPressContinuously = true;
    }

    for (;;) {
        SettingGet(&deviceSetting);

        // Expander inputs check
        if(GpioExpanderDriverIsInterruptSet()){
            ESP_LOGI(TAG, "the input on the expander has changed");
            GpioExpanderDriverGetInputPort(&inputPort);
            GpioExpanderDriverClearIrq();
            GpioExpanderDriverPrintInputStatus(inputPort);
        }

        // factory reset sequence detect
        if((inputPort.wifiSwitch != false) || (inputPort.limitSwitch3 != true)){
            isFactoryResetButtonsPressContinuously = false;
        }

        if((isFactoryResetButtonsPressContinuously == true) && (TimeDriverHasTimeElapsed(factorySequenceStartTime, DEVICEMANAGER_FACTORY_RESTART_TIMEOUT_MS))){
            deviceSetting.backFactorySetting = true;
            SettingSet(&deviceSetting);
            ESP_LOGI(TAG, "Factory reset start sequence");
        }

        // print ethernet status change
        if((deviceSetting.ethernetStatus != ethernetDriverGetStatus()) || (deviceSetting.ethernetStatus != deviceSettingOld.ethernetStatus)){
            deviceSetting.ethernetStatus = ethernetDriverGetStatus();
            ESP_LOGI(TAG, "Ethernet connection status change to %d", deviceSetting.ethernetStatus);

            // when we are connected to ethernet via cable we don't need wifi
            if(deviceSetting.ethernetStatus == ETHERNET_EVENT_CONNECTED){
                deviceSetting.restore.isWifiOn = false;
            }

            SettingSet(&deviceSetting);
        }

        // print wifi status change
        if(deviceSetting.wifiStatus != WifiGetStaStatus()){
            deviceSetting.wifiStatus = WifiGetStaStatus();
            ESP_LOGI(TAG, "Wifi connection status change to %d", deviceSetting.wifiStatus);
            SettingSet(&deviceSetting);
        }

        // wifi button operation
        WifiButtonOperation(&deviceSetting, &inputPort);

        if(deviceSetting.tryConnectToNewAp == true){
            // must be wifi led toggle for 15 sec
            LedToggleWifi(&deviceSetting);
            retryConnectToWifi = true;
        }

        int64_t newApConnectionTime = WifiGetNewApConnectionTime();
        if((deviceSetting.tryConnectToNewAp) && (newApConnectionTime != 0) && (deviceSetting.wifiMode == WIFI_MODE_APSTA)){
            if((deviceSetting.wifiStatus == WIFI_STATUS_STA_CONNECTED) || (TimeDriverHasTimeElapsed(newApConnectionTime,DEVICEMANAGER_WIFI_CONNECTION_TRY_NEW_AP_MS) == true)){
                SettingGet(&deviceSetting);
                deviceSetting.tryConnectToNewAp = false;
                SettingSet(&deviceSetting);
                
                if(deviceSetting.wifiStatus == WIFI_STATUS_STA_CONNECTED){
                    retryConnectToWifi = true;
                    deviceSetting.isConnectNewAp = true;
                    SettingSet(&deviceSetting);
            
                    WebServerStop();
                    WifiReinit();
                }else{
                    ESP_LOGI(TAG, "time to connect to new AP passed");
                    retryConnectToWifi = false;
                }
            }
        }

        if(deviceSetting.restore.isWifiOn != WifiRfEmit()){
            if(deviceSetting.restore.isWifiOn == true){
                ESP_LOGI(TAG, "user now wants to turn on wifi");
                WifiStart();
            }
            else{
                ESP_LOGI(TAG, "wifi is disabled because of wifi switch");
                WifiStop();
            }
        }

        if(deviceSetting.wifiMode != WifiModeGet()){
            deviceSetting.wifiMode = WifiModeGet();
            ESP_LOGI(TAG, "new wifi mode %d", deviceSetting.wifiMode);
        }

        // after ota veryfication 
        if((isVeryficationNeeded == true) && (TimeDriverHasTimeElapsed(newFirmwareTime, DEVICEMANAGER_NEW_FIRMWARE_VERYFICATION_TIMEOUT_MS))){
            ESP_LOGI(TAG, "veryfication pass");
            OtaMarkValid();
            ESP_LOGI(TAG, "Indicate that the running app is working well");

            isVeryficationNeeded = false;
        }

        // factore restart
        if(deviceSetting.backFactorySetting == true){
            RestoreFactoryDeviceSetting(&deviceSetting);
        }

        // device planned restart 
        if(deviceSetting.deviceReset == true){
            DeviceRestart(&deviceSetting);
        }
        
        // checking if an error occurred
        if(AlarmHandlingErrorCheck(&deviceSetting, &inputPort) == true){
            AlarmHandlingManagement(&deviceSetting);
        }
        else{
            if(GpioExpanderDriverIsBuzzerOn() == true){
                GpioExpanderDriverBuzzerOff();
            }
        }
        
        // checking if an alarm occurred
        if(AlarmHandlingWarningCheck(&deviceSetting) == true){
            // when it appears we work normally
        }

        // rewriting the contents of the esp32 timers to deviceSetting
        if(TimeDriverHasTimeElapsed(updtateTimerTime, DEVICEMANAGER_UPDATE_TIMERS_MS)){
            updtateTimerTime = TimeDriverGetSystemTickMs();

            TimerDriverUpdateTimerSetting(&deviceSetting);
            AlarmHandlingTimersWornOutCheck(&deviceSetting);

            SettingUpdateTimers(&deviceSetting);
            ESP_LOGI(TAG, "setting timers update");
        }

        // scheduler support
        if(SchedulerIsDeviceStatusUpdateNeeded(&deviceSetting) == true){
           SettingGet(&deviceSetting);
           SchedulerGetCurrentDeviceStatus(&deviceSetting);
           SettingUpdateDeviceStatus(&deviceSetting);
        }

        // touch panel operation
        bool settingsChange = TouchButtonStatus(&buttons);
        if(settingsChange == true){
            if(deviceSetting.restore.touchLock == false){
                SettingGet(&deviceSetting);
                TouchChangeDeviceSetting(&deviceSetting, &buttons);
                SettingUpdateDeviceStatus(&deviceSetting);
            }
            else{
                // if touch lock is set changing device setting using touch is impossible
                LedLockSequenceStart();
            }
        }

        UvLampManagement(&deviceSetting);

        settingsChange = IsSettingChange(&deviceSetting, &deviceSettingOld);
        if((runFirstTime == false) || (settingsChange == true)){
            ESP_LOGI(TAG, "setting change");
            LedChangeColor(&deviceSetting);
            FanLevelChange(&deviceSetting);

            SettingSet(&deviceSetting);
            runFirstTime = true;
        }
        
        UvLampExecute(&deviceSetting);

        // too frequent attempts to connect to wifi block wifi ap
        if(TimeDriverHasTimeElapsed(wifiConnectionTryTime, DEVICEMANAGER_WIFI_CONNECTION_TRY_INTERVAL_MS)){
            wifiConnectionTryTime = TimeDriverGetSystemTickMs();

            if((retryConnectToWifi == true) && (deviceSetting.wifiStatus == WIFI_STATUS_STA_DISCONNECTED)){
                ESP_LOGI(TAG, "trying to connect to wifi");
                WifiStaConnect();
            }
        }

        // important settings for the return when the power is cut off
        bool saveNvs = NeedSaveSettingToNvm(&deviceSetting, &deviceSettingNvs);
        
        // we can't save too often timers. Limited number of write cycles to nvs
        bool  IsTimersChanges = false;
        if(TimeDriverHasTimeElapsed(saveSettingTime, DEVICEMANAGER_SAVE_SETTING_MS)){
            saveSettingTime = TimeDriverGetSystemTickMs();
            IsTimersChanges = TimersChanges(&deviceSetting, &deviceSettingNvs);
        }

        // saving device status and timers to nvs
        if((saveNvs == true) || (IsTimersChanges == true)){
            SettingSave();
            ESP_LOGI(TAG, "Device setting save to NVS");
            memcpy(&deviceSettingNvs, &deviceSetting, sizeof(SettingDevice_t));
        }

        // print device setting
        if(TimeDriverHasTimeElapsed(updtateStatusTime, DEVICEMANAGER_UPDATE_STATUS_MS)){
            updtateStatusTime = TimeDriverGetSystemTickMs();

            PrintSetting(&deviceSetting);
            PrintStatus();
        }

        TestRunProcess();

        memcpy(&deviceSettingOld, &deviceSetting, sizeof(SettingDevice_t));
        vTaskDelay(DEVICEMANAGER_TASK_DELAY_MS);
    }
}

/******************************************************************************
                        PRIVATE FUNCTION IMPLEMENTATION
******************************************************************************/

static void PrintDeviceStatusInfo(void)
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    const esp_app_desc_t *app_desc = esp_ota_get_app_description();

    ESP_LOGI(TAG, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

    ESP_LOGI(TAG, "Firmware version %s", app_desc->version);
    ESP_LOGI(TAG, "Project name %s", app_desc->project_name);
    ESP_LOGI(TAG, "Compilation date %s time %s", app_desc->date, app_desc->time);
    ESP_LOGI(TAG, "Idf ver %s", app_desc->idf_ver);

    ESP_LOGI(TAG, "This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    ESP_LOGI(TAG, "silicon revision %d, ", chip_info.revision);

    ESP_LOGI(TAG, "%dMB %s flash", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    ESP_LOGI(TAG, "Minimum free heap size: %d bytes", esp_get_minimum_free_heap_size());

    ESP_LOGI(TAG, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
}

static void PrintRestartReason(void)
{
    esp_reset_reason_t reason = esp_reset_reason();

    switch(reason){
        case ESP_RST_UNKNOWN:
            ESP_LOGI(TAG, "Reset reason can not be determined");
            break;
        case ESP_RST_POWERON:
            ESP_LOGI(TAG, "Reset due to power-on event");
            break;
        case ESP_RST_EXT:
            ESP_LOGI(TAG, "Reset by external pin (not applicable for ESP32)");
            break;
        case ESP_RST_SW:
            ESP_LOGI(TAG, "Software reset via esp_restart");
            break;
        case ESP_RST_PANIC:
            ESP_LOGI(TAG, "Software reset due to exception/panic");
            break;
        case ESP_RST_INT_WDT:
            ESP_LOGI(TAG, "Reset (software or hardware) due to interrupt watchdog");
            break;
        case ESP_RST_TASK_WDT:
            ESP_LOGI(TAG, "Reset due to task watchdog");
            break;
        case ESP_RST_WDT:
            ESP_LOGI(TAG, "Reset due to other watchdogs");
            break;
        case ESP_RST_DEEPSLEEP:
            ESP_LOGI(TAG, "Reset after exiting deep sleep mode");
            break;
        case ESP_RST_BROWNOUT:
            ESP_LOGI(TAG, "Brownout reset (software or hardware)");
            break;
        case ESP_RST_SDIO:
            ESP_LOGI(TAG, "Reset over SDIO");
            break;
    }
}

static void PrintStatus(void)
{
    ESP_LOGI(TAG, "task number %d", uxTaskGetNumberOfTasks());
    ESP_LOGI(TAG, "available internal heap %d bytes", esp_get_free_internal_heap_size());
    ESP_LOGI(TAG, "minimum free heap size: %d bytes", esp_get_minimum_free_heap_size());
    ESP_LOGI(TAG, "");

    int16_t count = 0 ;
    FanTachoState_t status = FanGetTachoRevolutionsPerSecond(&count);
    ESP_LOGI(TAG, "fan status %d, speed %d [RPS]", status, count);

    uint32_t volt = UvLampGetMeanMiliVolt(UV_LAMP_1);
    ESP_LOGI(TAG, "Uv lamp 1 ballast mean %u [mV]", volt);

    volt = UvLampGetMeanMiliVolt(UV_LAMP_2);
    ESP_LOGI(TAG, "Uv lamp 2 ballast mean %u [mV]", volt);

    ESP_LOGI(TAG, "");
}

static void PrintSetting(SettingDevice_t* setting)
{
    ESP_LOGI(TAG, "Device Settings:");
    ESP_LOGI(TAG, "Time on device %s", TimeDriverGetLocalTimeStr());
    ESP_LOGI(TAG, "isOn %d", setting->restore.deviceStatus.isDeviceOn);
    ESP_LOGI(TAG, "fan level %d", (setting->restore.deviceStatus.fanLevel + 1));
    ESP_LOGI(TAG, "touch lock %s", (setting->restore.touchLock) ? "YES" : "NO");
    ESP_LOGI(TAG, "mode %s", (setting->restore.deviceMode == DEVICE_MODE_MANUAL) ? "MANUAL" : "AUTO");
    ESP_LOGI(TAG, "eko %s", (setting->restore.deviceStatus.isEkoOn == true) ? "ON" : "OFF");
    ESP_LOGI(TAG, "lamp 1 %s, lamp 2 %s", (setting->uvLamp1On == true) ? "ON" : "OFF", (setting->uvLamp2On == true) ? "ON" : "OFF");

    ESP_LOGI(TAG, "ethernet pcb added %s", (setting->ethernetPcbAdded == true) ? "YES" : "NO");
    ESP_LOGI(TAG, "ethernet status %d", setting->ethernetStatus);
    ESP_LOGI(TAG, "wifi on %s, status %d", (setting->restore.isWifiOn ? "YES" : "NO"), setting->wifiStatus);

    ESP_LOGI(TAG, "Alarm handling:");
    AlarmHandlingPrint(setting);

    ESP_LOGI(TAG, "timer hepa %lld [S]", TIMER_DRIVER_RAW_DATA_TO_SECOND(setting->restore.liveTime[TIMER_NAME_HEPA]));
    ESP_LOGI(TAG, "timer uv lamp 1 %lld [S]", TIMER_DRIVER_RAW_DATA_TO_SECOND(setting->restore.liveTime[TIMER_NAME_UV_LAMP_1]));
    ESP_LOGI(TAG, "timer uv lamp 2 %lld [S]", TIMER_DRIVER_RAW_DATA_TO_SECOND(setting->restore.liveTime[TIMER_NAME_UV_LAMP_2]));
    ESP_LOGI(TAG, "timer global on %lld [S]", TIMER_DRIVER_RAW_DATA_TO_SECOND(setting->restore.liveTime[TIMER_NAME_GLOBAL_ON]));
    ESP_LOGI(TAG, "");
}

static void RestoreFactoryDeviceSetting(SettingDevice_t* setting)
{
    if(setting->backFactorySetting == false){
        return;
    }
    setting->backFactorySetting = false;
    SettingSet(setting);

    // sequence start
    LedResetFactoryInformation();
    GpioExpanderDriverBuzzerOff();

    for(uint16_t idx = 0; idx <= 5; ++idx){
        GpioExpanderDriverBuzzerOn();
        vTaskDelay(100U);
        GpioExpanderDriverBuzzerOff();
        vTaskDelay(100U);
    }

    ESP_LOGI(TAG, "restore factory setting");
    
    bool res = true;

    setting->restore.deviceStatus.isDeviceOn = false;
    setting->restore.deviceStatus.fanLevel = FAN_LEVEL_1;
    setting->restore.deviceStatus.isEkoOn = false;

    setting->restore.touchLock = false;
    setting->restore.deviceMode = DEVICE_MODE_MANUAL;
    setting->restore.isWifiOn = true;

    res &= SettingSet(setting);
    ESP_LOGI(TAG, "set setting %d", res);

    Scheduler_t factoryScheduler = {};
    res &= FactorySettingsGetScheduler(&factoryScheduler);
    res &= SchedulerSetAll(&factoryScheduler);
    ESP_LOGI(TAG, "restore factory scheduler %d", res);

    iotHubClientStatus_t websocketSetting = {};
    res &= IotHubClientGetSetting(&websocketSetting);
    websocketSetting.isConnectedLeastOnce = false;
    res &= IotHubClientSetSetting(&websocketSetting);
    res &= IotHubClientSettingSave();
    ESP_LOGI(TAG, "turn on webserver %d", res);

    wifiSetting_t wifiSetting = {};
    res &= WifiSettingSave(&wifiSetting);
    ESP_LOGI(TAG, "clear wifi setting %d", res);

    vTaskDelay(1000U);

    McuDriverDeviceSafeRestart();
}

static void DeviceRestart(SettingDevice_t* setting)
{
    if(setting->deviceReset == false){
        return;
    }

    setting->deviceReset = false;
    SettingSet(setting);

    McuDriverDeviceSafeRestart();
}

static bool IsSettingChange(const SettingDevice_t* newSetting, const SettingDevice_t* oldSetting)
{
    SettingDevice_t toCmpNew = {};
    SettingDevice_t toCmpOld = {};

    memcpy(&toCmpNew, newSetting, sizeof(SettingDevice_t));
    memcpy(&toCmpOld, oldSetting, sizeof(SettingDevice_t));

    memset(toCmpNew.restore.liveTime, 0, (TIMER_NAME_COUNTER * sizeof(uint64_t)));
    memset(toCmpOld.restore.liveTime, 0, (TIMER_NAME_COUNTER * sizeof(uint64_t)));

    int cmpRes = memcmp(&toCmpNew, &toCmpOld, sizeof(SettingDevice_t));

    if(cmpRes == 0){
        return false;
    }

    return true;
}

static bool NeedSaveSettingToNvm(const SettingDevice_t* newSetting, const SettingDevice_t* oldSetting)
{
    if(newSetting->restore.deviceMode == DEVICE_MODE_MANUAL){
        if(newSetting->restore.deviceStatus.isDeviceOn != oldSetting->restore.deviceStatus.isDeviceOn){
        return true;
        }

        if(newSetting->restore.deviceStatus.fanLevel != oldSetting->restore.deviceStatus.fanLevel){
            return true;
        }
    }

    if(newSetting->restore.deviceMode != oldSetting->restore.deviceMode){
        return true;
    }

    if(newSetting->restore.touchLock != oldSetting->restore.touchLock){
        return true;
    }

    if(newSetting->restore.deviceStatus.isEkoOn != oldSetting->restore.deviceStatus.isEkoOn){
        return true;
    }

    if(newSetting->restore.isWifiOn != oldSetting->restore.isWifiOn){
        return true;
    }

    return false;
}

static bool TimersChanges(const SettingDevice_t* newSetting, const SettingDevice_t* oldSetting)
{
    int cmpRes = memcmp(newSetting->restore.liveTime, oldSetting->restore.liveTime, (TIMER_NAME_COUNTER * sizeof(uint64_t)));

    if(cmpRes == 0){
        return false;
    }

    return true;
}

static bool WifiButtonOperation(SettingDevice_t* setting, const GpioExpanderPinout_t *inputPort)
{
    static bool sIsWifiOnLock = false;

    // we are connected we cannot turn on the wifi
    if(setting->ethernetStatus == ETHERNET_EVENT_CONNECTED){
        return setting->restore.isWifiOn;
    }

    if(inputPort->wifiSwitch == false){
        if((sIsWifiOnLock == false)){
            if(setting->restore.isWifiOn == true){
                // User wants to turn off wifi
                setting->restore.isWifiOn = false;
            }else{
                setting->restore.isWifiOn = true;
            }
            SettingSet(setting);
        }
        sIsWifiOnLock = true;
    }
    else{
        sIsWifiOnLock = false;
    }

    return setting->restore.isWifiOn;
}