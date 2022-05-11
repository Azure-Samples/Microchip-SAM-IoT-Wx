/*
    \file   debug_print.c

    \brief  debug_console printer

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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "debug_print.h"
#include "definitions.h"

#define APP_PRINT_BUFFER_SIZE 512

static char                   printBuff[APP_PRINT_BUFFER_SIZE] __attribute__((aligned(4)));
static int                    printBuffPtr;
static OSAL_MUTEX_HANDLE_TYPE consoleMutex;
static debug_severity_t       debug_severity_filter    = SEVERITY_NONE;
static char                   debug_message_prefix[25] = "sn000000000000000000";
static volatile bool          debug_disabled = false;

char tmpBuf[APP_PRINT_BUFFER_SIZE];
char tmpFormat[APP_PRINT_BUFFER_SIZE];

void debug_init(const char* prefix)
{
    if (prefix)
    {
        debug_setPrefix(prefix);
    }

    debug_setSeverity(SEVERITY_ERROR);
    printBuffPtr = 0;
    OSAL_MUTEX_Create(&consoleMutex);
}

void debug_setSeverity(debug_severity_t debug_level)
{
    debug_severity_filter = debug_level;
}

debug_severity_t debug_getSeverity(void)
{
    return debug_severity_filter;
}

void debug_setPrefix(const char* prefix)
{
    strncpy(debug_message_prefix, prefix, sizeof(debug_message_prefix));
    debug_message_prefix[strlen(debug_message_prefix)] = 0;
}

void debug_disable(bool disable)
{
    // SYS_CONSOLE_Flush(0); // this is NOP
    debug_disabled = disable;

    if (disable == false)
    {
        SYS_CONSOLE_Message(0, "\r\n");
    }
}

// *****************************************************************************
// *****************************************************************************
// A workaround for OSAL mutex.
// OSAL Mutex without OS (e.g. FreeRTOS) does not honor wait/OSAL_WAIT_FOREVER.
// *****************************************************************************
// *****************************************************************************

void debug_mutex_lock(OSAL_MUTEX_HANDLE_TYPE* mutexID)
{

    while (OSAL_RESULT_TRUE != OSAL_MUTEX_Lock(mutexID, OSAL_WAIT_FOREVER))
    {
        // give a short delay (5ms)
        SYS_TIME_HANDLE tmrHandle = SYS_TIME_HANDLE_INVALID;

        if (SYS_TIME_SUCCESS != SYS_TIME_DelayMS(5, &tmrHandle))
        {
            return;
        }

        while (true != SYS_TIME_DelayIsComplete(tmrHandle))
        {
            continue;
        }
    }
}

void debug_printer(debug_severity_t debug_severity, debug_errorLevel_t error_level, const char* format, ...)
{
    size_t  len = 0;
    va_list args;

    if (debug_severity >= SEVERITY_NONE && debug_severity <= SEVERITY_TRACE)
    {
        if (debug_severity <= debug_severity_filter)
        {
            if (error_level < LEVEL_INFO)
                error_level = LEVEL_INFO;

            if (error_level > LEVEL_ERROR)
                error_level = LEVEL_ERROR;

            debug_mutex_lock(&consoleMutex);

            if (debug_disabled == false || error_level == LEVEL_ERROR) // always print error
            {
                sprintf(tmpFormat, "%s %s %s %s\r\n" CSI_RESET, debug_message_prefix, severity_strings[debug_severity], level_strings[error_level], format);

                va_start(args, format);
                len = vsnprintf(tmpBuf, APP_PRINT_BUFFER_SIZE, tmpFormat, args);
                va_end(args);

                if ((len > 0) && (len < APP_PRINT_BUFFER_SIZE))
                {
                    char* pBuf;
                    if ((len + printBuffPtr) > APP_PRINT_BUFFER_SIZE)
                    {
                        printBuffPtr = 0;
                    }

                    memcpy(&printBuff[printBuffPtr], tmpBuf, len);
                    pBuf                              = &printBuff[printBuffPtr];
                    printBuff[printBuffPtr + len + 1] = '\0';
                    printBuffPtr                      = (printBuffPtr + len + 3) & ~3;
                    SYS_CONSOLE_Write(0, pBuf, len);
                }
            }
            OSAL_MUTEX_Unlock(&consoleMutex);
        }
    }
}
