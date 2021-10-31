/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include "frame.h"
#include "led.h"
#include "azutil.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************
uint8_t  FRAME_buffer[DATAFRAME_NUMBYTES];
uint16_t FRAME_index = 0;
Data_Frame APP_rxDataFrame;

// *****************************************************************************
// *****************************************************************************
// Section: Helper Functions
// *****************************************************************************
// *****************************************************************************
static void SERCOM0_callback(uintptr_t context)
{
    if ( (FRAME_buffer[FRAMEIDX_CMD] == CMDCHAR_TELEMETRY_1) || 
         (FRAME_buffer[FRAMEIDX_CMD] == CMDCHAR_TELEMETRY_2) )
    {
        if (FRAME_index == HEADER_NUMBYTES)
        {
            APP_rxDataFrame.length = ( (FRAME_buffer[FRAMEIDX_LENMSB] << 8) + 
                    FRAME_buffer[FRAMEIDX_LENLSB] );
        }
        if (FRAME_index == (HEADER_NUMBYTES + APP_rxDataFrame.length))
        {
            APP_rxDataFrame.command = FRAME_buffer[FRAMEIDX_CMD];
            APP_rxDataFrame.index = FRAME_buffer[FRAMEIDX_INDEX];
            APP_rxDataFrame.payload = &FRAME_buffer[HEADER_NUMBYTES];
            FRAME_index = 0;
            LED_RED_Toggle_EX();
            send_telemetry_from_uart(APP_rxDataFrame.index, "Hello world!!!");
            //send_telemetry_from_uart(APP_rxDataFrame.index, (char*)APP_rxDataFrame.payload);
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

