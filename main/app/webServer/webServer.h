/**
 * @file webServer.h
 *
 * @brief Web Server header file
 *
 * @author matwal
 * @date 2021.08.25
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/*****************************************************************************
                       PUBLIC DEFINES / MACROS / ENUMS
*****************************************************************************/

/*****************************************************************************
                         PUBLIC INTERFACE DECLARATION
*****************************************************************************/

bool WebServerInit(void);

/** @brief Function to start web server
 *  
 */
void WebServerStart(void);

/** @brief Function to stop web server
 *  
 */
void WebServerStop(void);
