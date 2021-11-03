/*******************************************************************************
  Main Header File

  Company:
    Microchip Technology Inc.

  File Name:
    dti.h

 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Definitions
// *****************************************************************************
// *****************************************************************************

#define DTI_CMDCHAR_NUMBYTES 1
#define DTI_PARAM1_NUMBYTES 1
#define DTI_PAYLOADLEN_NUMBYTES 2
#define DTI_HEADER_NUMBYTES (DTI_CMDCHAR_NUMBYTES + DTI_PARAM1_NUMBYTES + DTI_PAYLOADLEN_NUMBYTES)
#define DTI_PAYLOADDATA_NUMBYTES 1024
#define NULLCHAR_NUMBYTES 1
#define DTI_MAXTOTAL_NUMBYTES (DTI_CMDCHAR_NUMBYTES + DTI_PARAM1_NUMBYTES + DTI_PAYLOADLEN_NUMBYTES + DTI_PAYLOADDATA_NUMBYTES + NULLCHAR_NUMBYTES)

/* Indices pointing to specific locations in the data frame */
#define DTI_pIDX_CMDCHAR 0
#define DTI_pIDX_PARAM1 1
#define DTI_pIDX_PAYLENMSB 2
#define DTI_pIDX_PAYLENLSB 3

/* Valid command byte values */
#define DTI_CMDCHAR_TELEMETRY_1 'T'
#define DTI_CMDCHAR_TELEMETRY_2 't'

#define CHAR_NULL '\0'

typedef struct {
    uint8_t  commandChar; // single char/byte corresponding to a valid command
    uint8_t  parameter1;  // index used to reference a specific parameter in the command
    uint16_t payloadLen;  // 16-bit length (in bytes) for the current payload
    uint8_t *payloadData; // pointer to the beginning of the payload array/string
} DTI_DataFrameInfo;

/*******************************************************************************
 End of File
*/
