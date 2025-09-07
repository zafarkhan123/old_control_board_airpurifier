#include <stdio.h>
#include "sdkconfig.h"

#include "config.h"

#include "device/deviceInit.h"
#include "production/deviceInitProduction.h"

void app_main(void)
{
    #if CFG_BUILD_TYPE != CFG_BUILD_PRODUCTION
        DeviceInit();
    #else
        DeviceInitProduction();
    #endif
}
