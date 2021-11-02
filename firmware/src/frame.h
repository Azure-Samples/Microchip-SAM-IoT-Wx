/*******************************************************************************
  Main Header File

  Company:
    Microchip Technology Inc.

  File Name:
    frame.h

 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <string.h>                     // Defines memcpy()

// *****************************************************************************
// *****************************************************************************
// Section: Definitions
// *****************************************************************************
// *****************************************************************************

#define FRAME_CMDCHAR_NUMBYTES 1
#define FRAME_PARAM1_NUMBYTES 1
#define FRAME_PAYLOADLEN_NUMBYTES 2
#define FRAME_HEADER_NUMBYTES (FRAME_CMDCHAR_NUMBYTES+FRAME_PARAM1_NUMBYTES+FRAME_PAYLOADLEN_NUMBYTES)
#define FRAME_PAYLOADDATA_NUMBYTES 1024
#define NULLCHAR_NUMBYTES 1
#define FRAME_MAXTOTAL_NUMBYTES (FRAME_CMDCHAR_NUMBYTES+FRAME_PARAM1_NUMBYTES+FRAME_PAYLOADLEN_NUMBYTES+FRAME_PAYLOADDATA_NUMBYTES+NULLCHAR_NUMBYTES)

/* Indeces pointing to specific locations in the data frame */
#define FRAME_pIDX_CMDCHAR 0
#define FRAME_pIDX_PARAM1 1
#define FRAME_pIDX_PAYLENMSB 2
#define FRAME_pIDX_PAYLENLSB 3

/* Valid command byte values */
#define FRAME_CMDCHAR_TELEMETRY_1 'T'
#define FRAME_CMDCHAR_TELEMETRY_2 't'

#define CHAR_NULL '\0'

typedef struct {
    uint8_t  commandChar; // single char/byte corresponding to a valid command
    uint8_t  parameter1;  // index used to reference a specific parameter in the command
    uint16_t payloadLen;  // 16-bit length (in bytes) for the current payload
    uint8_t *payloadData; // pointer to the beginning of the payload array/string
    bool     processRqst; // flag to request processing of the command
} FRAME_DataFrameInfo;

extern FRAME_DataFrameInfo FRAME_parsedInfo;
extern uint16_t FRAME_bufferPtr;

void FRAME_init(void);

/*******************************************************************************
 End of File
*/
