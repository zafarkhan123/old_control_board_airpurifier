# Protocols

Azure and IoT Hub provide the ability to register and connect individual IoT devices so that their telemetry data can be sent and received in the cloud. To help connect millions of devices in a secure and scalable manner without requiring human intervention we use IoT Hub Device Provisioning Service

## X.509 certificate chain

We need generate an X.509 certificate chain of three certificates to connect single devices to IoT Hub.

![](images/certChain.JPG)

Root certificate: You will complete proof of possession to verify the root certificate. This verification will enable DPS to trust that certificate and verify certificates signed by it.
Intermediate Certificate: It's common for intermediate certificates to be used to group devices logically by product lines, device location.
Device certificates: The device (leaf) certificates will be signed by the intermediate certificate and stored on the device along with its private key. Ideally these sensitive items would be stored securely with an HSM. Each device will present its certificate and private key, along with the certificate chain when attempting provisioning.

## Create and register a provisioning client 

The ICoN device to communicate to the Azure IoTHub must be created as a Provisioning Client. The creation step requires three parameters: 

1. IotHub URI: (example iconhubue.azure-devices.net) 
2. Scope_id  
3. Protocol: Prov_Device_MQTT_WS_Protocol In case of successful creation ,the device must be registered. 

## Connection and message exchange 

When the device is successfully registered in provisioning client procedure (1.2) a client to communicate with the existing IoT Hub must be created, using the device auth module (https://docs.microsoft.com/en-us/azure/iot-hub/iot-c-sdk-ref/iothub-client-ll-h/iothubclient-ll-createfromdeviceauth). The parameters: 

1. IoTHub hostname received in the registration process 
2. device Id received in the registration process 
3. Protocol: Prov_Device_MQTT_WS_Protocol 

The device to cloud (and vice-versa) communication varies depends on the reason of the communication (see device-to-cloud and cloud-to-device for details). Thus, the different schemas is planned to be used.

## Messages (API) 
- Device info Json
Send device id, device name, HW version and FW version. 
```
{  
    "MessageName": "deviceInfo",
    "DeviceId": “ICON_M_2022.02.0001”, 
    "HwVersion": "1.0",  
    "FwVersion": "1.0.0"
} 
```
The message is sent after the ICoN in two cases: 
1. Initial message (after successful connection to the cloud) 
2. After successfully upgrade 

### - Device status Json
The ICoN sends device status information
```
{  
    "MessageName": "deviceStatus", 
    "DeviceId": "ICON_M_2022.02.0001", 
    "Timestamp": 1628763832, // when the change of schedule is done  
    "DeviceMode": 0, # 0 - manual, 1 - automatic  
    "TotalOn": 2, // time when the device is turned on [in hours]  
    "TimUv1": 5, // percent of lamp life (in comparison with lifespan) 
    "TimUv2": 5, // percent of lamp life (in comparison with lifespan)  
    "TimHepa": 10, // percent of HEPA life 
    "Rtc": 1628763832, // unix timestamp  
    "FanLevel": 5, // 0 - off ,[1-5] – fan speed level  
    "EcoMode": false, // false - off, true - on  
    "AlarmCodes": [12, 101], 
    “EthernetOn”: false, // true if ICoN communicates to the cloud over ethernet, otherwise false  
    "TouchLock": false, // true if the touchpanel is on and operate 
    "WifiOn": true, //false if the Wifi is off by switch           
    "DeviceReset": true,  
    "ResetReason" : 1 
} 
```

### - Device location Json
The ICoN informs the cloud about its location info (send message) and The Cloud can changes location info on the ICoN.
```
{ 
    "MessageName": "deviceLocation",  
    "DeviceId": "ICON_M_2022.02.0001",  
    "Location": "Cystersów 19, Kraków",  
    "Room": "16"
}
```
### - Device scheduler Json
The ICoN sends and receives weekly schedule by the ICoN to the cloud. 
```
{ 
    "MessageName": "deviceSchedule",  
    "DeviceId": "ICON_M_2022.02.0001", 
    "Timestamp": 1628763832, // when the change of schedule is done 
    "Monday":{   
        "fan":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],  
        "eco":[false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false]}, 
    "Tuesday":{   
        "fan":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],  
        "eco":[false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false]},  
    "Wednesday":{   
        "fan":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],  
        "eco":[false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false]},  
    "Thursday":{   
        "fan":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],  
        "eco":[false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false]},  
    "Friday":{   
        "fan":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],  
        "eco":[false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false]},  
    "Saturday":{   
        "fan":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],  
        "eco":[false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false]},  
    "Sunday":{   
        "fan":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],  
        "eco":[false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false]} 
} 
```

### - Device mode Json
The ICoN receives the new device mode, fan speed, eco mode or touchpad lock/unlock request. 
```
{  
    "MessageName": "deviceMode", 
    "DeviceId": "ICON_M_2022.02.0001", 
    "Timestamp": 1628763832, // when the change is requested  
    "DeviceMode": 0, // 0 - manual, 1 - automatic  
    "FanLevel": 5, // 0 - off ,[1-5]  
    "EcoMode": false, // false - off, true - on  
    "TouchLock": false // false means touch is to be unlocked 
}
```

### - Device service setting Json
The  ICoN receives service request (UV lamp/HEPA filter replacement, RT clock reset request or change of the UV lamp/HEPA filter parameter) 
```
{  
    "MessageName": "deviceService", 
    "DeviceId": "ICON_M_2022.02.0001", 
    "DeviceReset": 0, // 0 - no, 1 - yes  
    "TimUv1Reload": 0, // 0 - no, 1 - yes  
    "TimUv2Reload": 0, // 0 - no, 1 - yes  
    "TimHepaReload": 0, // 0 - no, 1 - yes  
    "ScheduleReset": 0, // 0 - no, 1 - yes  
    "RtcSet": 1628763832, // unix timestamp 
    "HepaLivespan": 9000, // in hours 
    "UvLivespan": 8000, // in hours 
    "HepaWarning": 8100, // in hours 
    "UvWarning": 7200, // in hours 
    "UtcTimeoffset": 2 // UTC offset (in hours) 
}
```

### - Firmware update Json
The ICoN receives the upgrade firmaware request. In case of successful upgrade the ICoN sends the deviceInfo message with the new FwVersion. 
```
{ 
    "MessageName": "deviceUpdate",  
    "DeviceId": "ICON_M_2022.02.0001",  
    "NewDeviceId": "2", 
    "FwVersion": "2.1.1",  
    "FwPackageURI": "https://example.com",  
    "FwPackageCheckValue": 3421780262
}
```