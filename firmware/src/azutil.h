#ifndef _AZUTIL_H
#define _AZUTIL_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include "definitions.h"
#include "mqtt/mqtt_core/mqtt_core.h"
#include "azure/core/az_span.h"
#include "azure/core/az_json.h"
#include "azure/core/az_http.h"
#include "azure/iot/az_iot_pnp_client.h"
#include "services/iot/cloud/cloud_service.h"
#include "led.h"
#include "services/iot/cloud/mqtt_packetPopulation/mqtt_iothub_packetPopulate.h"

#define LED_NO_CHANGE (-1)

#define RETURN_ERR_IF_FAILED(ret)  \
    do                             \
    {                              \
        if (az_result_failed(ret)) \
        {                          \
            return ret;            \
        }                          \
    } while (0)

#define RETURN_IF_FAILED(ret)      \
    do                             \
    {                              \
        if (az_result_failed(ret)) \
        {                          \
            return;                \
        }                          \
    } while (0)

#define RETURN_ERR_WITH_MESSAGE_IF_FAILED(ret, msg) \
    do                                              \
    {                                               \
        if (az_result_failed(ret))                  \
        {                                           \
            debug_printError(msg);                  \
            return ret;                             \
        }                                           \
    } while (0)

#define RETURN_WITH_MESSAGE_IF_FAILED(ret, msg) \
    do                                          \
    {                                           \
        if (az_result_failed(ret))              \
        {                                       \
            debug_printError(msg);              \
            return;                             \
        }                                       \
    } while (0)

#define EXIT_WITH_MESSAGE_IF_FAILED(ret, msg) \
    do                                        \
    {                                         \
        if (az_result_failed(ret))            \
        {                                     \
            debug_printError(msg);            \
            exit(1);                          \
        }                                     \
    } while (0)

typedef union
{
    struct
    {
        unsigned short version_found : 1;
        unsigned short isInitialGet : 1;
        unsigned short max_temp_updated : 1;
        unsigned short telemetry_interval_found : 1;
        unsigned short yellow_led_found : 1;
        unsigned short reserved : 11;
    };
    unsigned short AsUSHORT;
} twin_update_flag_t;

typedef struct
{
    twin_update_flag_t flag;
    int32_t            version_num;
    int32_t            desired_led_yellow;   // 1 = On, 2 = Off, 3 = Blink
    int8_t             reported_led_red;
    int8_t             reported_led_blue;
    int8_t             reported_led_green;
} twin_properties_t;

typedef union
{
    struct
    {
        unsigned sw0 : 1;
        unsigned sw1 : 1;
        unsigned reserved : 14;
    };
    unsigned AsUSHORT;
} button_press_flag_t;

typedef struct
{
    uint32_t            sw0_press_count;
    uint32_t            sw1_press_count;
    button_press_flag_t flag;
} button_press_data_t;

extern button_press_data_t button_press_data;

void init_twin_data(
    twin_properties_t* twin_properties);

az_result start_json_object(
    az_json_writer* jw,
    az_span         az_span_buffer);

az_result end_json_object(
    az_json_writer* jw);

az_result append_json_property_int32(
    az_json_writer* jw,
    az_span         property_name_span,
    int32_t         property_val);

az_result append_jason_property_string(
    az_json_writer* jw,
    az_span         property_name_span,
    az_span         property_val_span);

// int mqtt_publish_message(
//     char*   topic,
//     az_span payload,
//     int     qos);
void check_button_status(void);

az_result send_telemetry_message(void);

az_result send_reported_property(
    twin_properties_t* twin_properties);

az_result process_direct_method_command(
    uint8_t*                           payload,
    az_iot_pnp_client_command_request* command_request);

az_result process_device_twin_property(
    uint8_t*           topic,
    uint8_t*           payload,
    twin_properties_t* twin_properties);

void update_leds(twin_properties_t* twin_properties);

OSAL_RESULT MUTEX_Lock(
    OSAL_MUTEX_HANDLE_TYPE* mutexID,
    uint16_t                waitMS);
#endif