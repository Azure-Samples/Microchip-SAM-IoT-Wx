 // Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

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

#define LED_TWIN_NO_CHANGE (-1)
#define LED_TWIN_ON        (1)
#define LED_TWIN_OFF       (2)
#define LED_TWIN_BLINK     (3)

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
        uint16_t version_found : 1;
        uint16_t is_initial_get : 1;
        uint16_t telemetry_interval_found : 1;
        uint16_t yellow_led_found : 1;
        uint16_t reserved : 12;
    };
    uint16_t as_uint16;
} twin_update_flag_t;

typedef struct
{
    twin_update_flag_t flag;
    int32_t            version_num;
    int32_t            desired_led_yellow;
    int8_t             reported_led_red;
    int8_t             reported_led_blue;
    int8_t             reported_led_green;
} twin_properties_t;

typedef union
{
    struct
    {
        uint16_t sw0 : 1;
        uint16_t sw1 : 1;
        uint16_t reserved : 14;
    };
    uint16_t as_uint16;
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

az_result append_json_property_string(
    az_json_writer* jw,
    az_span         property_name_span,
    az_span         property_val_span);


void check_button_status(void);

az_result send_telemetry_message(void);

az_result send_reported_property(
    twin_properties_t* twin_properties);

az_result process_direct_method_command(
    uint8_t*                           payload,
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    az_iot_pnp_client_command_request* command_request
#else
    az_iot_hub_client_method_request* method_request
#endif
    );

az_result process_device_twin_property(
    uint8_t*           topic,
    uint8_t*           payload,
    twin_properties_t* twin_properties);

void update_leds(twin_properties_t* twin_properties);

#endif
