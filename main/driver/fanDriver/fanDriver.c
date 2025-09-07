/**
 * @file fanDriver.c
 *
 * @brief Fan driver source file
 *
 * @dir fanDriver
 * @brief Fan driver folder
 *
 * @author matfio
 * @date 2021.08.24
 * @version v1.0
 *
 * @copyright 2021 Fideltronik R&D - all rights reserved.
 */

#include "config.h"

#include "esp_log.h"

#include "driver/ledc.h"
#include "driver/pcnt.h"
#include "fanDriver.h"


/*****************************************************************************
                          PRIVATE DEFINES / MACROS
 *****************************************************************************/

#define PWM_SPEED_MODE LEDC_LOW_SPEED_MODE
#define PWM_TIMER_NUMBER LEDC_TIMER_1

#define COUNTER_UNIT_NUMBER PCNT_UNIT_0

/*****************************************************************************
                     PRIVATE STRUCTS / ENUMS / VARIABLES
*****************************************************************************/

static ledc_channel_config_t sPwmChannel = {
    .channel    = LEDC_CHANNEL_0,
    .duty       = 0,
    .gpio_num   = CFG_FAN_PWM_GPIO_PIN,
    .speed_mode = PWM_SPEED_MODE,
    .hpoint     = 0,
    .timer_sel  = PWM_TIMER_NUMBER
};

/*****************************************************************************
                           INTERFACE IMPLEMENTATION
*****************************************************************************/

bool FanDriverInit(void)
{
     ledc_timer_config_t pwmTimer = {
        .duty_resolution = LEDC_TIMER_12_BIT,   // resolution of PWM duty
        .freq_hz = 5U * 1000U,                  // frequency of PWM signal
        .speed_mode = PWM_SPEED_MODE,           // timer mode
        .timer_num = PWM_TIMER_NUMBER,          // timer index
        .clk_cfg = LEDC_AUTO_CLK,               // Auto select the source clock
    };

     /* Initialize LEDC unit */
    ledc_timer_config(&pwmTimer);
    ledc_channel_config(&sPwmChannel);

    pcnt_config_t counter = {
        // Set PCNT input signal and control GPIOs
        .pulse_gpio_num = CFG_FAN_TACHO_GPIO_PIN,
        .channel = PCNT_CHANNEL_0,
        .unit = COUNTER_UNIT_NUMBER,
        // What to do on the positive
        .pos_mode = PCNT_COUNT_INC,   // Count up on the positive edge
    };

     /* Initialize PCNT unit */
    pcnt_unit_config(&counter);

    /* Initialize PCNT's counter */
    pcnt_counter_pause(COUNTER_UNIT_NUMBER);
    pcnt_counter_clear(COUNTER_UNIT_NUMBER);

    /* Start counting */
    pcnt_counter_resume(COUNTER_UNIT_NUMBER);

    return true;
}

bool FanDriverSetDuty(uint32_t duty)
{
    if(duty > 0x0fff)
    {
        return false;
    }

    ledc_set_duty(sPwmChannel.speed_mode, sPwmChannel.channel, duty);
    ledc_update_duty(sPwmChannel.speed_mode, sPwmChannel.channel);

    return true;
}

int16_t FanDriverGetTachoCount(void)
{
    int16_t count;

    pcnt_get_counter_value(COUNTER_UNIT_NUMBER, &count);
    pcnt_counter_clear(COUNTER_UNIT_NUMBER);

    return count;
}