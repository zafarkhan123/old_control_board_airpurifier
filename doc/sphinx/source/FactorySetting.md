# Factory settings

This utility is designed to create instances of factory NVS partition images on a per-device basis for mass manufacturing purposes. The NVS partition images are created from CSV files containing user-provided configurations and values.

| key | type |value (example) | description |
| --- | ---- | -------------- | ----------- |
| deviceName | string | ICON_M_2022.01.0001 | device name (wifi network) |
| deviceType | string | M | device type |
| hardwareVersion | string | 1.0.0 | HW version |
| location | string | USA | location |
| fanPwmLevel1 | u32 | 819 | fan level 1 |
| fanPwmLevel2 | u32 | 1638 | fan level 2 |
| fanPwmLevel3 | u32 | 2457 | fan level 3 |
| fanPwmLevel4 | u32 | 3276 | fan level 4 |
| fanPwmLevel5 | u32 | 4095 | fan level 5 |
| hepaLifeTime | u32 | 20000 | lifetime of hepa filters [Hours] |
| hepaWarnTime | u32 | 18000 | alarm about the impending end of life of the hepa filters [Hours] |
| uvLampLifeTime | u32 | 10000 | uv lamp life time [Hours] |
| uvLampWarnTime | u32 | 9000 | alarm about the impending end of life of the uv lamp [Hours] |
| uvLampOffMin | u32 | 1800 | ballast switched off minimum value [mV] |
| uvLampOffMax | u32 | 2200 | ballast switched off maximum value [mV] |
| uvLampOnMin | u32 | 2500 | ballast switched on minimum value [mV] |
| uvLampOnMax | u32 | 4500 | ballast switched on maximum value [mV] |
| logoLed | u32 | 16777215 | color of 2 LOGO LEDs |
| servPass | string | service | password to unlock service settings on the website |
| diagnPass | string | diagnostics | password to unlock diagnostic settings on the website|
| cloudAddress | string | iconhubeu.azure-devices.net | Provisioning address in Azure Iot Hub |
| cloudPort | u32 | 443 | Azure IoT Hub provisioning port |
| idScope | string | 0ne000F6E03 | ID Scope for icon provisioning |
| scheduler | binary file | data/scheduler_blob.bin | binary file with the initial settings of the schedule |
| rootCert | text file | cert/root/azure-iot-icon.root.ca.cert.pem | root certificate for iCoN IoTHub |
| interCert | text file | cert/intermediate/azure-iot-icon.intermediate.cert.pem | intermediate for iCoN IoTHub |
| clientCert | text file | cert/1/ICON_TEST_2022_03.0001.cert.pem | unique device certificate |
| clientKey | text file | cert/1/ICON_TEST_2022_03.0001.key.pem | unique private key of the device |