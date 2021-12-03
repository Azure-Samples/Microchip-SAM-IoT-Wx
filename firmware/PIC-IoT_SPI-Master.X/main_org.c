/**
  Generated main.c file from MPLAB Code Configurator
  @Company
    Microchip Technology Inc.
  @File Name
    main.c
  @Summary
    This is the generated main.c using PIC24 / dsPIC33 / PIC32MM MCUs.
  @Description
    This source file provides main entry point for system initialization and application code development.
    Generation Information :
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.169.0
        Device            :  PIC24FJ128GA705
    The generated drivers are tested against the following:
        Compiler          :  XC16 v1.50
        MPLAB 	          :  MPLAB X v5.40
*/

/*
    (c) 2020 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/

/**
  Section: Included Files
*/
#include "main.h"

static volatile bool SW0_buttonPressed = false;
static volatile bool SW1_buttonPressed = false;
static volatile bool isTMR1TimerExpired = false;
uint8_t  APP_txBuffer[TXBUFF_NUMBYTES];
size_t   APP_txBufSize = TXBUFF_NUMBYTES;
uint8_t  APP_rxBuffer[RXBUFF_NUMBYTES];
size_t   APP_rxBufSize = RXBUFF_NUMBYTES;
uint8_t  APP_dataFrameIndex = CMD_STR_INDEX_MIN;
uint16_t APP_dataFramesSent = 0;

void APP_prepareDataFrame(uint8_t index)
{
    uint16_t counter;
    
    APP_txBuffer[0] = 't';
    APP_txBuffer[1] = index;
    APP_txBuffer[2] = ((PAYLOAD_NUMBYTES >> 8) & 0x00FF);
    APP_txBuffer[3] = (PAYLOAD_NUMBYTES & 0x00FF);
    for (counter = HEADER_NUMBYTES; counter < (HEADER_NUMBYTES+PAYLOAD_NUMBYTES); counter++)
    {
        APP_txBuffer[counter] = index + 38;
    }
    APP_txBuffer[HEADER_NUMBYTES+0] = 'S';
    APP_txBuffer[HEADER_NUMBYTES+1] = 'T';
    APP_txBuffer[HEADER_NUMBYTES+2] = 'A';
    APP_txBuffer[HEADER_NUMBYTES+3] = 'R';
    APP_txBuffer[HEADER_NUMBYTES+4] = 'T';
    APP_txBuffer[TXBUFF_NUMBYTES-3] = 'E';
    APP_txBuffer[TXBUFF_NUMBYTES-2] = 'N';
    APP_txBuffer[TXBUFF_NUMBYTES-1] = 'D';    
}

void APP_sendDataFrame(void)
{
    uint16_t total = 0;

    APP_prepareDataFrame(APP_dataFrameIndex);
    do
    {
        total = SPI2_Exchange8bitBuffer( &APP_txBuffer[total], (TXBUFF_NUMBYTES - total), &APP_rxBuffer[total]);
    } while(total < TXBUFF_NUMBYTES);
}

static void SW0_interruptHandler(void)
{
    SW0_buttonPressed = true;
}

static void SW1_interruptHandler(void)
{
    SW1_buttonPressed = true;
}

static void TMR1_interruptHandler(void)
{
    isTMR1TimerExpired = true;
    LED_GREEN_Toggle();
    LED_YELLOW_Toggle();
}

/*
                         Main application
 */
int main(void)
{
    // initialize the device
    SYSTEM_Initialize();

    /* Register callback function for SW0 interrupt */
    BUTTON_SW0_SetInterruptHandler(&SW0_interruptHandler);
    
    /* Register callback function for SW1 interrupt */
    BUTTON_SW1_SetInterruptHandler(&SW1_interruptHandler);
        
    /* Register callback function for TMR1 interrupt */
    TMR1_SetInterruptHandler(&TMR1_interruptHandler);
    
    /* Initialize LEDs */
    LED_BLUE_SetLow();
    LED_GREEN_SetLow();
    LED_YELLOW_SetHigh();
    
    while (1)
    {
        // Add your application code
        if (SW0_buttonPressed == true)
        {
            while (APP_dataFramesSent < DATAFRAMES_TOTAL2SEND_SW0)
            {
                if (isTMR1TimerExpired == true)
                {
                    isTMR1TimerExpired = false;
                    APP_sendDataFrame();
                    APP_dataFramesSent++;
                    APP_dataFrameIndex++;
                    if (APP_dataFrameIndex > CMD_STR_INDEX_MAX)
                    {
                        APP_dataFrameIndex = CMD_STR_INDEX_MIN;    
                    }
                    LED_RED_Toggle();
                }
            }
            APP_dataFramesSent = 0;
            SW0_buttonPressed = false;
        }
        if (SW1_buttonPressed == true)
        {
            while (APP_dataFramesSent < DATAFRAMES_TOTAL2SEND_SW1)
            {
                if (isTMR1TimerExpired == true)
                {
                    isTMR1TimerExpired = false;
                    APP_sendDataFrame();
                    APP_dataFramesSent++;
                    APP_dataFrameIndex++;
                    if (APP_dataFrameIndex > CMD_STR_INDEX_MAX)
                    {
                        APP_dataFrameIndex = CMD_STR_INDEX_MIN;    
                    }
                    LED_RED_Toggle();
                }
            }
            APP_dataFramesSent = 0;
            SW1_buttonPressed = false;
        }
    }

    return 1;
}
/**
 End of File
*/

