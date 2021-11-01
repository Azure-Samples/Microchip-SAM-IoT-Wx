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

#define FRAME_CMD_NUMBYTES 1
#define FRAME_IDX_NUMBYTES 1
#define FRAME_LEN_NUMBYTES 2
#define FRAME_HEADER_NUMBYTES (FRAME_CMD_NUMBYTES+FRAME_IDX_NUMBYTES+FRAME_LEN_NUMBYTES)
#define FRAME_PAYLOAD_NUMBYTES 1024
#define NULLCHAR_NUMBYTES 1
#define FRAME_MAXTOTAL_NUMBYTES (FRAME_CMD_NUMBYTES+FRAME_IDX_NUMBYTES+FRAME_LEN_NUMBYTES+FRAME_PAYLOAD_NUMBYTES+NULLCHAR_NUMBYTES)

/* Indeces pointing to specific locations in the data frame */
#define FRAME_pIDX_CMD 0
#define FRAME_pIDX_INDEX 1
#define FRAME_pIDX_LENMSB 2
#define FRAME_pIDX_LENLSB 3

/* Valid command byte values */
#define FRAME_CMDCHAR_TELEMETRY_1 'T'
#define FRAME_CMDCHAR_TELEMETRY_2 't'

#define CHAR_NULL '\0'

extern uint16_t FRAME_index;

typedef struct {
    uint8_t  command; // single char/byte corresponding to a valid command
    uint8_t  index;   // index used to reference a specific parameter in the command
    uint16_t length;  // 16-bit length (in bytes) for the current payload
    uint8_t *payload; // pointer to the beginning of the payload array/string
} FRAME_DataFrame;

void FRAME_init(void);

/*******************************************************************************
 End of File
*/
