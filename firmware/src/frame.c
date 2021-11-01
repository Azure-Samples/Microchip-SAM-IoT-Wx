/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    frame.c

  Summary:
    This file contains the functions for the FRAME software module.

 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include "definitions.h"
#include "frame.h"
#include "led.h"
#include "azutil.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************
uint8_t  FRAME_buffer[FRAME_MAXTOTAL_NUMBYTES];
uint16_t FRAME_index = 0;
FRAME_DataFrame APP_rxDataFrame;

// *****************************************************************************
// *****************************************************************************
// Section: Helper Functions
// *****************************************************************************
// *****************************************************************************
static void SERCOM0_callback(uintptr_t context)
{
    if ( (FRAME_buffer[FRAME_pIDX_CMD] == FRAME_CMDCHAR_TELEMETRY_1) || 
         (FRAME_buffer[FRAME_pIDX_CMD] == FRAME_CMDCHAR_TELEMETRY_2) )
    // Did we receive a valid command byte at the start of the incoming buffer?
    {
        if (FRAME_index == FRAME_HEADER_NUMBYTES)
        // Did we just receive the LSB of the 16-bit length parameter?
        {
            APP_rxDataFrame.length = ( (FRAME_buffer[FRAME_pIDX_LENMSB] << 8) + 
                    FRAME_buffer[FRAME_pIDX_LENLSB] ); // Word = (MSB + LSB)
        }
        if (FRAME_index == (FRAME_HEADER_NUMBYTES + APP_rxDataFrame.length))
        // Did we just receive the final byte of the payload data?
        {
            FRAME_buffer[FRAME_index] = CHAR_NULL; // Terminate the array/string
            FRAME_index = 0; // Reset pointer to the beginning of the buffer
            APP_rxDataFrame.command = FRAME_buffer[FRAME_pIDX_CMD];
            APP_rxDataFrame.index = FRAME_buffer[FRAME_pIDX_INDEX];
            APP_rxDataFrame.payload = &FRAME_buffer[FRAME_HEADER_NUMBYTES];
            process_telemetry_command(APP_rxDataFrame.index, (char*)APP_rxDataFrame.payload);
        }
    }
}

void FRAME_init(void)
{
    SERCOM0_SPI_CallbackRegister(&SERCOM0_callback, (uintptr_t)NULL);
}

/*******************************************************************************
 End of File
*/

