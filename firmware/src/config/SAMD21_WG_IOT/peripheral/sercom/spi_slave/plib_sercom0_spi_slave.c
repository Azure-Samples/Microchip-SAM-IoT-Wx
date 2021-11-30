/*******************************************************************************
  SERIAL COMMUNICATION SERIAL PERIPHERAL INTERFACE(SERCOM0_SPI) PLIB

  Company
    Microchip Technology Inc.

  File Name
    plib_sercom0_spi_slave.c

  Summary
    SERCOM0_SPI PLIB Slave Implementation File.

  Description
    This file defines the interface to the SERCOM SPI Slave peripheral library.
    This library provides access to and control of the associated
    peripheral instance.

  Remarks:
    None.

*******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2020 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

#include "plib_sercom0_spi_slave.h"
#include <string.h>
#include "dti.h"
#include "../../azutil.h"
#include "../../led.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************
uint16_t DTI_bufferPtr = 0;

// *****************************************************************************
// *****************************************************************************
// Section: MACROS Definitions
// *****************************************************************************
// *****************************************************************************
#define SERCOM0_SPI_READ_BUFFER_SIZE            256
#define SERCOM0_SPI_WRITE_BUFFER_SIZE           128

static uint8_t SERCOM0_SPI_ReadBuffer[SERCOM0_SPI_READ_BUFFER_SIZE];
static uint8_t SERCOM0_SPI_WriteBuffer[SERCOM0_SPI_WRITE_BUFFER_SIZE];
static uint8_t DTI_cmdBuffer[DTI_MAXTOTAL_NUMBYTES];
static DTI_DataFrameInfo DTI_parsedInfo;

/* Global object to save SPI Exchange related data  */
static SPI_SLAVE_OBJECT sercom0SPISObj;

// *****************************************************************************
// *****************************************************************************
// Section: SERCOM0_SPI Slave Implementation
// *****************************************************************************
// *****************************************************************************
// *****************************************************************************

void SERCOM0_SPI_Initialize(void)
{
    /* CHSIZE - 8_BIT
     * PLOADEN - 1
     *  SSDE - 1 
     * RXEN - 1
     */
    SERCOM0_REGS->SPIS.SERCOM_CTRLB = SERCOM_SPIS_CTRLB_CHSIZE_8_BIT | SERCOM_SPIS_CTRLB_PLOADEN_Msk | SERCOM_SPIS_CTRLB_RXEN_Msk | SERCOM_SPIS_CTRLB_SSDE_Msk;

    /* Wait for synchronization */
    while(SERCOM0_REGS->SPIS.SERCOM_SYNCBUSY);

    /* Mode - SPI Slave
     * IBON - 1 (Set immediately upon buffer overflow)
     * DOPO - PAD2
     * DIPO - PAD0
     * CPHA - TRAILING_EDGE
     * COPL - IDLE_LOW
     * DORD - MSB
     * ENABLE - 1
     */
    SERCOM0_REGS->SPIS.SERCOM_CTRLA = SERCOM_SPIS_CTRLA_MODE_SPI_SLAVE | SERCOM_SPIS_CTRLA_IBON_Msk | SERCOM_SPIS_CTRLA_DOPO_PAD2 | SERCOM_SPIS_CTRLA_DIPO_PAD0 | SERCOM_SPIS_CTRLA_CPOL_IDLE_LOW | SERCOM_SPIS_CTRLA_CPHA_TRAILING_EDGE | SERCOM_SPIS_CTRLA_DORD_MSB | SERCOM_SPIS_CTRLA_ENABLE_Msk ;

    /* Wait for synchronization */
    while(SERCOM0_REGS->SPIS.SERCOM_SYNCBUSY);

    sercom0SPISObj.rdInIndex = 0;
    sercom0SPISObj.wrOutIndex = 0;
    sercom0SPISObj.nWrBytes = 0;
    sercom0SPISObj.errorStatus = SPI_SLAVE_ERROR_NONE;
    sercom0SPISObj.callback = NULL ;
    sercom0SPISObj.transferIsBusy = false ;

    SERCOM0_REGS->SPIS.SERCOM_INTENSET = SERCOM_SPIS_INTENSET_SSL_Msk | SERCOM_SPIS_INTENCLR_RXC_Msk;
}

/* For 9-bit mode, the "size" must be specified in terms of 16-bit words */
size_t SERCOM0_SPI_Read(void* pRdBuffer, size_t size)
{
    uint8_t intState = SERCOM0_REGS->SPIS.SERCOM_INTENSET;
    size_t rdSize = size;

    SERCOM0_REGS->SPIS.SERCOM_INTENCLR = intState;

    if (rdSize > sercom0SPISObj.rdInIndex)
    {
        rdSize = sercom0SPISObj.rdInIndex;
    }

    memcpy(pRdBuffer, SERCOM0_SPI_ReadBuffer, rdSize);

    SERCOM0_REGS->SPIS.SERCOM_INTENSET = intState;

    return rdSize;
}

/* For 9-bit mode, the "size" must be specified in terms of 16-bit words */
size_t SERCOM0_SPI_Write(void* pWrBuffer, size_t size )
{
    uint8_t intState = SERCOM0_REGS->SPIS.SERCOM_INTENSET;
    size_t wrSize = size;

    SERCOM0_REGS->SPIS.SERCOM_INTENCLR = intState;

    if (wrSize > SERCOM0_SPI_WRITE_BUFFER_SIZE)
    {
        wrSize = SERCOM0_SPI_WRITE_BUFFER_SIZE;
    }

    memcpy(SERCOM0_SPI_WriteBuffer, pWrBuffer, wrSize);

    sercom0SPISObj.nWrBytes = wrSize;
    sercom0SPISObj.wrOutIndex = 0;

    while ((SERCOM0_REGS->SPIS.SERCOM_INTFLAG & SERCOM_SPIS_INTFLAG_DRE_Msk) && (sercom0SPISObj.wrOutIndex < sercom0SPISObj.nWrBytes))
    {
        SERCOM0_REGS->SPIS.SERCOM_DATA = SERCOM0_SPI_WriteBuffer[sercom0SPISObj.wrOutIndex++];
    }

    /* Restore interrupt enable state and also enable DRE interrupt to start pre-loading of DATA register */
    SERCOM0_REGS->SPIS.SERCOM_INTENSET = (intState | SERCOM_SPIS_INTENSET_DRE_Msk);

    return wrSize;
}

/* For 9-bit mode, the return value is in terms of 16-bit words */
size_t SERCOM0_SPI_ReadCountGet(void)
{
    return sercom0SPISObj.rdInIndex;
}

/* For 9-bit mode, the return value is in terms of 16-bit words */
size_t SERCOM0_SPI_ReadBufferSizeGet(void)
{
    return SERCOM0_SPI_READ_BUFFER_SIZE;
}

/* For 9-bit mode, the return value is in terms of 16-bit words */
size_t SERCOM0_SPI_WriteBufferSizeGet(void)
{
    return SERCOM0_SPI_WRITE_BUFFER_SIZE;
}

void SERCOM0_SPI_CallbackRegister(SERCOM_SPI_SLAVE_CALLBACK callBack, uintptr_t context )
{
    sercom0SPISObj.callback = callBack;

    sercom0SPISObj.context = context;
}

/* The status is returned busy during the period the chip-select remains asserted */
bool SERCOM0_SPI_IsBusy(void)
{
    return sercom0SPISObj.transferIsBusy;
}


SPI_SLAVE_ERROR SERCOM0_SPI_ErrorGet(void)
{
    SPI_SLAVE_ERROR errorStatus = sercom0SPISObj.errorStatus;

    sercom0SPISObj.errorStatus = SPI_SLAVE_ERROR_NONE;

    return errorStatus;
}

void SERCOM0_SPI_InterruptHandler(void)
{
    uint8_t txRxData;

    uint8_t intFlag = SERCOM0_REGS->SPIS.SERCOM_INTFLAG;
            
    if(SERCOM0_REGS->SPIS.SERCOM_INTFLAG & SERCOM_SPIS_INTFLAG_SSL_Msk)
    {
        /* Clear the SSL flag and enable TXC interrupt */
        SERCOM0_REGS->SPIS.SERCOM_INTFLAG = SERCOM_SPIS_INTFLAG_SSL_Msk;
        SERCOM0_REGS->SPIS.SERCOM_INTENSET = SERCOM_SPIS_INTENSET_TXC_Msk;
        sercom0SPISObj.rdInIndex = 0;
        sercom0SPISObj.transferIsBusy = true;
    }

    if (SERCOM0_REGS->SPIS.SERCOM_STATUS & SERCOM_SPIS_STATUS_BUFOVF_Msk)
    {
        /* Save the error to report it to application later, when the transfer is complete (TXC = 1) */
        sercom0SPISObj.errorStatus = SERCOM_SPIS_STATUS_BUFOVF_Msk;

        /* Clear the status register */
        SERCOM0_REGS->SPIS.SERCOM_STATUS = SERCOM_SPIS_STATUS_BUFOVF_Msk;

        /* Flush out the received data until RXC flag is set */
        while(SERCOM0_REGS->SPIS.SERCOM_INTFLAG & SERCOM_SPIS_INTFLAG_RXC_Msk)
        {
            txRxData = SERCOM0_REGS->SPIS.SERCOM_DATA;
        }

        /* Clear the Error Interrupt Flag */
        SERCOM0_REGS->SPIS.SERCOM_INTFLAG = SERCOM_SPIS_INTFLAG_ERROR_Msk;
    }

    if(SERCOM0_REGS->SPIS.SERCOM_INTFLAG & SERCOM_SPIS_INTFLAG_RXC_Msk)
    {
        /* Reading DATA register will also clear the RXC flag */
        txRxData = SERCOM0_REGS->SPIS.SERCOM_DATA;
        
        /***** [---START---] DTI Module Processing [---START---] *****/
        // Save the current received data byte into buffer
        DTI_cmdBuffer[DTI_bufferPtr] = txRxData;
        // Did we just receive the command character?
        if (DTI_bufferPtr == DTI_pIDX_CMDCHAR)
        {
            DTI_parsedInfo.commandChar = txRxData;
        }
        // Did we just receive the parameter1 byte?
        if (DTI_bufferPtr == DTI_pIDX_PARAM1)
        {
            DTI_parsedInfo.parameter1 = txRxData;
        }
        // Did we just receive the LSB of the 16-bit length parameter?
        if (DTI_bufferPtr == DTI_pIDX_PAYLENLSB)
        {
            DTI_parsedInfo.payloadLen = ( (DTI_cmdBuffer[DTI_pIDX_PAYLENMSB] << 8) + txRxData);
        }
        // Increment pointer for next incoming byte
        DTI_bufferPtr++;
        
        // Did we just receive the final byte of the payload data?
        if (DTI_bufferPtr == (DTI_HEADER_NUMBYTES + DTI_parsedInfo.payloadLen))
        {
            DTI_cmdBuffer[DTI_bufferPtr] = CHAR_NULL; // Terminate the array/string
            DTI_bufferPtr = 0; // Reset pointer to the beginning of the buffer
            DTI_parsedInfo.payloadData = &DTI_cmdBuffer[DTI_HEADER_NUMBYTES];
            if ( (DTI_parsedInfo.commandChar == DTI_CMDCHAR_TELEMETRY_1) || 
                 (DTI_parsedInfo.commandChar == DTI_CMDCHAR_TELEMETRY_2) )
            {
                    process_telemetry_command(DTI_parsedInfo.parameter1, 
                            (char*)DTI_parsedInfo.payloadData);
                    LED_YELLOW_Toggle_EX();
            }
        }
        /****** [---END---] DTI Module Processing [---END---] ********/
        
        if (sercom0SPISObj.rdInIndex < SERCOM0_SPI_READ_BUFFER_SIZE)
        {
            SERCOM0_SPI_ReadBuffer[sercom0SPISObj.rdInIndex++] = txRxData;
        }
    }

    if(SERCOM0_REGS->SPIS.SERCOM_INTFLAG & SERCOM_SPIS_INTFLAG_DRE_Msk)
    {
        if (sercom0SPISObj.wrOutIndex < sercom0SPISObj.nWrBytes)
        {
            txRxData = SERCOM0_SPI_WriteBuffer[sercom0SPISObj.wrOutIndex++];

            /* Before writing to DATA register (which clears TXC flag), check if TXC flag is set */
            if(SERCOM0_REGS->SPIS.SERCOM_INTFLAG & SERCOM_SPIS_INTFLAG_TXC_Msk)
            {
                intFlag = SERCOM_SPIS_INTFLAG_TXC_Msk;
            }
            SERCOM0_REGS->SPIS.SERCOM_DATA = txRxData;
        }
        else
        {
            /* Disable DRE interrupt. The last byte sent by the master will be shifted out automatically */
            SERCOM0_REGS->SPIS.SERCOM_INTENCLR = SERCOM_SPIS_INTENCLR_DRE_Msk;
        }
    }

    if(intFlag & SERCOM_SPIS_INTFLAG_TXC_Msk)
    {
        /* Clear TXC flag */
        SERCOM0_REGS->SPIS.SERCOM_INTFLAG = SERCOM_SPIS_INTFLAG_TXC_Msk;

        sercom0SPISObj.transferIsBusy = false;

        sercom0SPISObj.wrOutIndex = 0;
        sercom0SPISObj.nWrBytes = 0;

        if(sercom0SPISObj.callback != NULL)
        {
            sercom0SPISObj.callback(sercom0SPISObj.context);
        }
    }
}
