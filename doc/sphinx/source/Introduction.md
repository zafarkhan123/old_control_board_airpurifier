# Introduction

## Brief description
The scope of the project is to create the Firmaware of the new generation of the iCON Air Cleaner. The scope of the work includes
1. Configuration file (factory partition) uploaded to each device with the possibility of individual parameter setting 
  * Serial number 
  * PWM setting for 5 speed level 
  * Default schedule 
2. Timetable (Schedule) 
  * Option to turn off/on the schedule 
  * Device work according to the schedule 
3. Limit switch 
  * 2 x HEPA filters 
  * Pre filter 
4. Fan 
  * Fan speed reading (from tacho) 
  * Setting 5 fan speed 
5. UV lamp 
  * Control on/off lamps in different order 
  * Handling the occurrence of lamp fail 
  * Check that the contactors are not stuck together 
6. RGB Leds 
7. Touch panel 
  * Support for 3 buttons 
  * Button combination that generates some action (restart, …) 
  * Chip interrupt handling 
8. RTC 
  * Read, write to external RTC chip 
9. Battery voltage measurement (RTC battery) 
10. FT Service Tool 
11. Software for self-test of the device at the final assembly stage
12. Access point 
  * OTA 
  * Setting a password to access wifi (nice to have) 
  * Setting user schedule 
  * Service option 
13. Counters 
  * UV lamp time left to replace 
  * HEPA filter time left to replace 
  * Global device on counter 
14. Gpio expander support 
15. Saving the settings to the non-volatile memory (every 30 minutes) or when power failure 
16. Main thread 
  * Device management as a whole 
  * Checking available resources 
17. Communicaion to Cloud
  * Status (filter, lamps, etc) 
  * OTA 
  * Timetable (schedule) - setting, getting 
  * Service options 

