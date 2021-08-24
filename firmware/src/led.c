/*
    \file   led.c

    \brief  Manage board LED's

    (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#include <stdbool.h>
#include "led.h"
#include "debug_print.h"

led_status_t led_status;

#define LED_50ms_INTERVAL   50L
#define LED_100ms_INTERVAL  100L
#define LED_400ms_INTERVAL  400L
#define LED_ON_INTERVAL     200L
#define LED_TOGGLE_INTERVAL 3000L

void blink_task(uintptr_t context);

SYS_TIME_HANDLE blinkTimer_blue   = SYS_TIME_HANDLE_INVALID;
SYS_TIME_HANDLE blinkTimer_green  = SYS_TIME_HANDLE_INVALID;
SYS_TIME_HANDLE blinkTimer_yellow = SYS_TIME_HANDLE_INVALID;
SYS_TIME_HANDLE blinkTimer_red    = SYS_TIME_HANDLE_INVALID;
SYS_TIME_HANDLE toggleTimer_red   = SYS_TIME_HANDLE_INVALID;

#define CSI_RESET   "\33[0m"
#define CSI_RED     "\33[31m"
#define CSI_GREEN   "\33[32m"
#define CSI_YELLOW  "\33[33m"
#define CSI_BLUE    "\33[34m"
#define CSI_INVERSE "\33[7m"
#define CSI_NORMAL  "\33[27m"
#define CSI_CLS     "\33[2J"
#define CSI_HOME    "\33[1;1H"

const char* debug_led_state[] =
    {
        "Off",
        "On",
        "Blink(Fast)",
        "N/A",
        "Blink(Slow)"};

void LED_MSDelay(uint32_t ms);

static void testSequence(uint8_t ledState)
{
    if (LED_OFF == ledState)
    {
        LED_BLUE_SetHigh_EX();
        LED_MSDelay(LED_50ms_INTERVAL);
        LED_GREEN_SetHigh_EX();
        LED_MSDelay(LED_50ms_INTERVAL);
        LED_YELLOW_SetHigh_EX();
        LED_MSDelay(LED_50ms_INTERVAL);
        LED_RED_SetHigh_EX();
        LED_MSDelay(LED_50ms_INTERVAL);
    }
    else
    {
        LED_BLUE_SetLow_EX();
        LED_MSDelay(LED_50ms_INTERVAL);
        LED_GREEN_SetLow_EX();
        LED_MSDelay(LED_50ms_INTERVAL);
        LED_YELLOW_SetLow_EX();
        LED_MSDelay(LED_50ms_INTERVAL);
        LED_RED_SetLow_EX();
        LED_MSDelay(LED_50ms_INTERVAL);
    }
}

void LED_test(void)
{
    testSequence(LED_ON);
    testSequence(LED_OFF);
}

void LED_init(void)
{
    led_status.change_flag.as_uint16 = LED_FLAG_EMPTY;
    led_status.state_flag.as_uint16  = LED_FLAG_EMPTY;
    led_status.state_flag.blue      = LED_STATE_OFF;
    LED_BLUE_SetHigh_EX();

    led_status.state_flag.green = LED_STATE_OFF;
    LED_GREEN_SetHigh_EX();

    led_status.state_flag.yellow = LED_STATE_OFF;
    LED_YELLOW_SetHigh_EX();

    led_status.state_flag.red = LED_STATE_OFF;
    LED_RED_SetHigh_EX();
}

void LED_MSDelay(uint32_t ms)
{
    SYS_TIME_HANDLE tmrHandle = SYS_TIME_HANDLE_INVALID;

    if (SYS_TIME_SUCCESS != SYS_TIME_DelayMS(ms, &tmrHandle))
    {
        return;
    }

    while (true != SYS_TIME_DelayIsComplete(tmrHandle))
    {
        continue;
    }
}

void blink_task(uintptr_t context)
{
    led_number_t led = (led_number_t)context;

    switch (led)
    {
        case LED_BLUE:
            LED_BLUE_Toggle_EX(); /* toggle LED_BLUE output */
            break;
        case LED_GREEN:
            LED_GREEN_Toggle_EX(); /* toggle LED_GREEN output */
            break;
        case LED_YELLOW:
            LED_YELLOW_Toggle_EX(); /* toggle LED_YELLOW output */
            break;
        case LED_RED:
            LED_RED_Toggle_EX(); /* toggle LED_RED output */
            break;
    }

    return;
}

/*************************************************
*
* Blue LED : WiFi Connection
*
*************************************************/
void LED_SetBlue(led_set_state_t newState)
{
    if (led_status.state_flag.blue == newState)
    {
        return;
    }

#if (CFG_LED_DEBUG == 1)
    debug_printInfo(CSI_BLUE "LED-B: %s => %s", debug_led_state[led_status.state_flag.blue], debug_led_state[newState]);
#endif

    switch ((int32_t)led_status.state_flag.blue)
    {
        case LED_STATE_OFF:
        case LED_STATE_HOLD:
            if ((newState & LED_STATE_BLINK_FAST) != 0)
            {
                if (blinkTimer_blue == SYS_TIME_HANDLE_INVALID)
                {
                    blinkTimer_blue = SYS_TIME_CallbackRegisterMS(blink_task, LED_BLUE, LED_100ms_INTERVAL, SYS_TIME_PERIODIC);
                }
                else
                {
                    SYS_TIME_RESULT result;
                    result = SYS_TIME_TimerReload(blinkTimer_blue, 0, SYS_TIME_MSToCount(LED_100ms_INTERVAL), blink_task, LED_BLUE, SYS_TIME_PERIODIC);

                    if (result != SYS_TIME_SUCCESS)
                    {
                        debug_printError("LED-B: Failed to reload timer");
                        break;
                    }
                    else
                    {
                        SYS_TIME_TimerStart(blinkTimer_blue);
                    }
                }
            }
            else if ((newState & LED_STATE_BLINK_SLOW) != 0)
            {
                if (blinkTimer_blue == SYS_TIME_HANDLE_INVALID)
                {
                    blinkTimer_blue = SYS_TIME_CallbackRegisterMS(blink_task, LED_BLUE, LED_100ms_INTERVAL, SYS_TIME_PERIODIC);
                }
                else
                {
                    SYS_TIME_RESULT result;
                    result = SYS_TIME_TimerReload(blinkTimer_blue, 0, SYS_TIME_MSToCount(LED_ON_INTERVAL), blink_task, LED_BLUE, SYS_TIME_PERIODIC);

                    if (result != SYS_TIME_SUCCESS)
                    {
                        debug_printError("LED-B: Failed to reload timer");
                        break;
                    }
                    else
                    {
                        SYS_TIME_TimerStart(blinkTimer_blue);
                    }
                }
            }

            break;

        case LED_STATE_BLINK_FAST:

            if (newState == LED_STATE_HOLD || newState == LED_STATE_OFF)
            {
                SYS_TIME_TimerStop(blinkTimer_blue);
            }
            break;

        case LED_STATE_BLINK_SLOW:

            if (newState == LED_STATE_HOLD || newState == LED_STATE_OFF)
            {
                SYS_TIME_TimerStop(blinkTimer_blue);
            }

            break;

        default:
            break;
    }

    if (newState == LED_STATE_HOLD)
    {
        LED_BLUE_SetLow_EX();
    }
    else if (newState == LED_STATE_OFF)
    {
        LED_BLUE_SetHigh_EX();
    }

    led_status.state_flag.blue  = newState;
    led_status.change_flag.blue = 1;
}

/*************************************************
*
* Green LED
*
*************************************************/
void LED_SetGreen(led_set_state_t newState)
{
    if (led_status.state_flag.green == newState)
    {
        return;
    }

#if (CFG_LED_DEBUG == 1)
    debug_printInfo(CSI_GREEN "LED-G: %s => %s", debug_led_state[led_status.state_flag.green], debug_led_state[newState]);
#endif

    switch ((int32_t)led_status.state_flag.green)
    {
        case LED_STATE_OFF:
        case LED_STATE_HOLD:

            if ((newState & LED_STATE_BLINK_FAST) != 0)
            {
                if (blinkTimer_green == SYS_TIME_HANDLE_INVALID)
                {
                    blinkTimer_green = SYS_TIME_CallbackRegisterMS(blink_task, LED_GREEN, LED_100ms_INTERVAL, SYS_TIME_PERIODIC);
                }
                else
                {
                    SYS_TIME_RESULT result;
                    result = SYS_TIME_TimerReload(blinkTimer_green, 0, SYS_TIME_MSToCount(LED_100ms_INTERVAL), blink_task, LED_GREEN, SYS_TIME_PERIODIC);

                    if (result != SYS_TIME_SUCCESS)
                    {
                        debug_printError("LED-G: Failed to reload timer");
                        break;
                    }
                    else
                    {
                        SYS_TIME_TimerStart(blinkTimer_green);
                    }
                }
            }
            else if ((newState & LED_STATE_BLINK_SLOW) != 0)
            {
                if (blinkTimer_green == SYS_TIME_HANDLE_INVALID)
                {
                    blinkTimer_green = SYS_TIME_CallbackRegisterMS(blink_task, LED_GREEN, LED_100ms_INTERVAL, SYS_TIME_PERIODIC);
                }
                else
                {
                    SYS_TIME_RESULT result;
                    result = SYS_TIME_TimerReload(blinkTimer_green, 0, SYS_TIME_MSToCount(LED_ON_INTERVAL), blink_task, LED_GREEN, SYS_TIME_PERIODIC);

                    if (result != SYS_TIME_SUCCESS)
                    {
                        debug_printError("LED-G: Failed to reload timer");
                        break;
                    }
                    else
                    {
                        SYS_TIME_TimerStart(blinkTimer_green);
                    }
                }
            }

            break;

        case LED_STATE_BLINK_FAST:

            if (newState == LED_STATE_HOLD || newState == LED_STATE_OFF)
            {
                SYS_TIME_TimerStop(blinkTimer_green);
            }
            break;

        case LED_STATE_BLINK_SLOW:

            if (newState == LED_STATE_HOLD || newState == LED_STATE_OFF)
            {
                SYS_TIME_TimerStop(blinkTimer_green);
            }

            break;

        default:
            break;
    }

    if (newState == LED_STATE_HOLD)
    {
        LED_GREEN_SetLow_EX();
    }
    else if (newState == LED_STATE_OFF)
    {
        LED_GREEN_SetHigh_EX();
    }

    led_status.state_flag.green  = newState;
    led_status.change_flag.green = 1;
}

/*************************************************
*
* Yellow LED
*
*************************************************/
void LED_SetYellow(led_set_state_t newState)
{
    if (led_status.state_flag.yellow == newState)
    {
        return;
    }

#if (CFG_LED_DEBUG == 1)
    debug_printInfo(CSI_YELLOW "LED-Y: %s => %s", debug_led_state[led_status.state_flag.yellow], debug_led_state[newState]);
#endif

    switch ((int32_t)led_status.state_flag.yellow)
    {
        case LED_STATE_OFF:
        case LED_STATE_HOLD:
            if ((newState & (LED_STATE_BLINK_FAST | LED_STATE_BLINK_SLOW)) != 0)
            {
                if (blinkTimer_yellow == SYS_TIME_HANDLE_INVALID)
                {
                    blinkTimer_yellow = SYS_TIME_CallbackRegisterMS(blink_task, LED_YELLOW, LED_ON_INTERVAL, SYS_TIME_PERIODIC);
                }

                if (blinkTimer_yellow == SYS_TIME_HANDLE_INVALID)
                {
                    debug_printError("LED-Y: Failed to create timer");
                }
                else
                {
                    SYS_TIME_TimerStart(blinkTimer_yellow);
                }
            }

            break;

        case LED_STATE_BLINK_FAST:
        case LED_STATE_BLINK_SLOW:

            if (newState == LED_STATE_HOLD || newState == LED_STATE_OFF)
            {
                SYS_TIME_TimerStop(blinkTimer_yellow);
            }
            break;

        default:
            break;
    }

    if (newState == LED_STATE_HOLD)
    {
        LED_YELLOW_SetLow_EX();
    }
    else if (newState == LED_STATE_OFF)
    {
        LED_YELLOW_SetHigh_EX();
    }

    led_status.state_flag.yellow  = newState;
    led_status.change_flag.yellow = 1;
}

/*************************************************
*
* Red LED
*
*************************************************/
void LED_SetRed(led_set_state_t newState)
{
    if (led_status.state_flag.red == newState)
    {
        return;
    }

#if (CFG_LED_DEBUG == 1)
    debug_printInfo(CSI_RED "LED-R: %s => %s", debug_led_state[led_status.state_flag.red], debug_led_state[newState]);
#endif

    switch ((int32_t)led_status.state_flag.red)
    {
        case LED_STATE_OFF:
        case LED_STATE_HOLD:

            if ((newState & LED_STATE_BLINK_FAST) != 0)
            {
                if (blinkTimer_red == SYS_TIME_HANDLE_INVALID)
                {
                    blinkTimer_red = SYS_TIME_CallbackRegisterMS(blink_task, LED_RED, LED_100ms_INTERVAL, SYS_TIME_PERIODIC);
                }
                else
                {
                    SYS_TIME_RESULT result;
                    result = SYS_TIME_TimerReload(blinkTimer_red, 0, SYS_TIME_MSToCount(LED_100ms_INTERVAL), blink_task, LED_RED, SYS_TIME_PERIODIC);

                    if (result != SYS_TIME_SUCCESS)
                    {
                        debug_printError("LED-R: Failed to reload timer");
                        break;
                    }
                    else
                    {
                        SYS_TIME_TimerStart(blinkTimer_red);
                    }
                }
            }
            else if ((newState & LED_STATE_BLINK_SLOW) != 0)
            {
                if (blinkTimer_red == SYS_TIME_HANDLE_INVALID)
                {
                    blinkTimer_red = SYS_TIME_CallbackRegisterMS(blink_task, LED_RED, LED_100ms_INTERVAL, SYS_TIME_PERIODIC);
                }
                else
                {
                    SYS_TIME_RESULT result;
                    result = SYS_TIME_TimerReload(blinkTimer_red, 0, SYS_TIME_MSToCount(LED_ON_INTERVAL), blink_task, LED_RED, SYS_TIME_PERIODIC);

                    if (result != SYS_TIME_SUCCESS)
                    {
                        debug_printError("LED-R: Failed to reload timer");
                        break;
                    }
                    else
                    {
                        SYS_TIME_TimerStart(blinkTimer_red);
                    }
                }
            }

            break;

        case LED_STATE_BLINK_FAST:

            if (newState == LED_STATE_HOLD || newState == LED_STATE_OFF)
            {
                SYS_TIME_TimerStop(blinkTimer_red);
            }
            break;

        case LED_STATE_BLINK_SLOW:

            if (newState == LED_STATE_HOLD || newState == LED_STATE_OFF)
            {
                SYS_TIME_TimerStop(blinkTimer_red);
            }

            break;

        default:
            break;
    }

    if (newState == LED_STATE_HOLD)
    {
        LED_RED_SetLow_EX();
    }
    else if (newState == LED_STATE_OFF)
    {
        LED_RED_SetHigh_EX();
    }

    led_status.state_flag.red  = newState;
    led_status.change_flag.red = 1;
}

void LED_ToggleRed(void)
{
    switch (led_status.state_flag.red)
    {
        case LED_STATE_OFF:
        case LED_STATE_HOLD:
            LED_RED_Toggle_EX();
            toggleTimer_red = SYS_TIME_CallbackRegisterMS(blink_task, LED_RED, LED_TOGGLE_INTERVAL, SYS_TIME_SINGLE);
            break;

        default:
            break;
    }
}

void LED_SetWiFi(led_indicator_name_t state)
{
    switch (state)
    {
        case LED_INDICATOR_OFF:
            LED_SetBlue(LED_STATE_OFF);
            break;
        case LED_INDICATOR_PENDING:
            LED_SetBlue(LED_STATE_BLINK_SLOW);
            break;
        case LED_INDICATOR_SUCCESS:
            LED_SetBlue(LED_STATE_HOLD);
            break;
        case LED_INDICATOR_ERROR:
            LED_SetBlue(LED_STATE_BLINK_FAST);
            LED_SetRed(LED_STATE_HOLD);
            break;
    }
}

void LED_SetCloud(led_indicator_name_t state)
{
    switch (state)
    {
        case LED_INDICATOR_OFF:
            LED_SetGreen(LED_STATE_OFF);
            break;
        case LED_INDICATOR_PENDING:
            LED_SetGreen(LED_STATE_BLINK_SLOW);
            break;
        case LED_INDICATOR_SUCCESS:
            LED_SetGreen(LED_STATE_HOLD);
            break;
        case LED_INDICATOR_ERROR:
            LED_SetGreen(LED_STATE_BLINK_FAST);
            LED_SetRed(LED_STATE_HOLD);
            break;
    }
}
