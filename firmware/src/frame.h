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

#define COMMAND_NUMBYTES 1
#define INDEX_NUMBYTES 1
#define LENGTH_NUMBYTES 2
#define HEADER_NUMBYTES (COMMAND_NUMBYTES+INDEX_NUMBYTES+LENGTH_NUMBYTES)
#define PAYLOAD_NUMBYTES 1024
#define NULLCHAR_NUMBYTES 1
#define DATAFRAME_NUMBYTES (COMMAND_NUMBYTES+INDEX_NUMBYTES+LENGTH_NUMBYTES+PAYLOAD_NUMBYTES+NULLCHAR_NUMBYTES)

/* Indeces pointing to specific locations in the data frame */
#define FRAMEIDX_CMD 0
#define FRAMEIDX_INDEX 1
#define FRAMEIDX_LENMSB 2
#define FRAMEIDX_LENLSB 3

/* Valid command byte values */
#define CMDCHAR_TELEMETRY_1 'T'
#define CMDCHAR_TELEMETRY_2 't'

#define CHAR_NULL '\0'

extern uint16_t FRAME_index;

typedef struct {
    uint8_t  command; // single char/byte corresponding to a valid command
    uint8_t  index;   // index used to reference a specific parameter in the command
    uint16_t length;  // 16-bit length (in bytes) for the current payload
    uint8_t *payload; // pointer to the beginning of the payload array/string
} Data_Frame;

void FRAME_init(void);

/*******************************************************************************
 End of File
*/
