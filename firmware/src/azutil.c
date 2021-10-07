// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "azutil.h"
#include "nmdrv.h"

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
extern az_iot_pnp_client pnp_client;
#else
extern az_iot_hub_client iothub_client;
#endif
extern volatile uint32_t telemetryInterval;

// used by led.c to communicate LED state changes
extern led_status_t led_status;

extern uint16_t packet_identifier;

userdata_status_t userdata_status;

static char pnp_telemetry_topic_buffer[128];
static char pnp_telemetry_payload_buffer[128];

static char pnp_property_topic_buffer[128];
static char pnp_property_payload_buffer[256];

static char command_topic_buffer[128];
static char command_resp_buffer[256];

// Plug and Play Connection Values
static uint32_t request_id_int = 0;
static char     request_id_buffer[16];

// IoT Plug and Play properties
#ifndef IOT_PLUG_AND_PLAY_MODEL_ID
static const az_span iot_hub_property_desired         = AZ_SPAN_LITERAL_FROM_STR("desired");
static const az_span iot_hub_property_desired_version = AZ_SPAN_LITERAL_FROM_STR("$version");
#endif

static const az_span telemetry_name_temperature_span = AZ_SPAN_LITERAL_FROM_STR("temperature");
static const az_span telemetry_name_light_span       = AZ_SPAN_LITERAL_FROM_STR("light");

// Telemetry Interval writable property
static const az_span property_telemetry_interval_span = AZ_SPAN_LITERAL_FROM_STR("telemetryInterval");

// Button Press
button_press_data_t button_press_data = {0};
static char         button_event_buffer[128];

static const az_span event_name_button_event_span = AZ_SPAN_LITERAL_FROM_STR("button_event");
static const az_span event_name_button_name_span  = AZ_SPAN_LITERAL_FROM_STR("button_name");
static const az_span event_name_button_sw0_span   = AZ_SPAN_LITERAL_FROM_STR("SW0");
static const az_span event_name_button_sw1_span   = AZ_SPAN_LITERAL_FROM_STR("SW1");
static const az_span event_name_press_count_span  = AZ_SPAN_LITERAL_FROM_STR("press_count");

// LED Properties
static const az_span led_blue_property_name_span   = AZ_SPAN_LITERAL_FROM_STR("led_b");
static const az_span led_green_property_name_span  = AZ_SPAN_LITERAL_FROM_STR("led_g");
static const az_span led_yellow_property_name_span = AZ_SPAN_LITERAL_FROM_STR("led_y");
static const az_span led_red_property_name_span    = AZ_SPAN_LITERAL_FROM_STR("led_r");

// IP Address property
static const az_span ip_address_property_name_span = AZ_SPAN_LITERAL_FROM_STR("ipAddress");

// Firmware Version Property
static const az_span fw_version_property_name_span = AZ_SPAN_LITERAL_FROM_STR("firmwareVersion");

// Command
static const az_span command_name_reboot_span              = AZ_SPAN_LITERAL_FROM_STR("reboot");
static const az_span command_reboot_delay_payload_span     = AZ_SPAN_LITERAL_FROM_STR("delay");
static const az_span command_status_span                   = AZ_SPAN_LITERAL_FROM_STR("status");
static const az_span command_resp_success_span             = AZ_SPAN_LITERAL_FROM_STR("Success");
static const az_span command_resp_empty_delay_payload_span = AZ_SPAN_LITERAL_FROM_STR("Delay time is empty. Specify 'delay' in period format (PT5S for 5 sec)");
static const az_span command_resp_bad_payload_span         = AZ_SPAN_LITERAL_FROM_STR("Delay time in wrong format. Specify 'delay' in period format (PT5S for 5 sec)");
static const az_span command_resp_error_processing_span    = AZ_SPAN_LITERAL_FROM_STR("Error processing command");
static const az_span command_resp_not_supported_span       = AZ_SPAN_LITERAL_FROM_STR("{\"Status\":\"Unsupported Command\"}");

static const az_span command_name_sendMsg_span               = AZ_SPAN_LITERAL_FROM_STR("sendMsg");
static const az_span command_sendMsg_payload_span            = AZ_SPAN_LITERAL_FROM_STR("sendMsgString");
static const az_span command_resp_empty_sendMsg_payload_span = AZ_SPAN_LITERAL_FROM_STR("Message string is empty. Specify string.");
static const az_span command_resp_alloc_error_sendMsg_span   = AZ_SPAN_LITERAL_FROM_STR("Failed to allocate memory for the message.");

static SYS_TIME_HANDLE reboot_task_handle = SYS_TIME_HANDLE_INVALID;

/**********************************************
* Initialize twin property data structure
**********************************************/
void init_twin_data(twin_properties_t* twin_properties)
{
    twin_properties->flag.as_uint16     = LED_FLAG_EMPTY;
    twin_properties->version_num        = 0;
    twin_properties->desired_led_yellow = LED_TWIN_NO_CHANGE;
    twin_properties->reported_led_red   = LED_TWIN_NO_CHANGE;
    twin_properties->reported_led_blue  = LED_TWIN_NO_CHANGE;
    twin_properties->reported_led_green = LED_TWIN_NO_CHANGE;
}

/**************************************
 Start JSON_BUILDER for JSON Document
 This creates a new JSON with "{"
*************************************/
az_result start_json_object(
    az_json_writer* jw,
    az_span         az_span_buffer)
{
    RETURN_ERR_IF_FAILED(az_json_writer_init(jw, az_span_buffer, NULL));
    RETURN_ERR_IF_FAILED(az_json_writer_append_begin_object(jw));
    return AZ_OK;
}

/**********************************************
* End JSON_BUILDER for JSON Document
* This adds "}" to the JSON
**********************************************/
az_result end_json_object(
    az_json_writer* jw)
{
    RETURN_ERR_IF_FAILED(az_json_writer_append_end_object(jw));
    return AZ_OK;
}

/**********************************************
*	Add a JSON key-value pair with int32 data
*	e.g. "property_name" : property_val (number)
**********************************************/
az_result append_json_property_int32(
    az_json_writer* jw,
    az_span         property_name_span,
    int32_t         property_val)
{
    RETURN_ERR_IF_FAILED(az_json_writer_append_property_name(jw, property_name_span));
    RETURN_ERR_IF_FAILED(az_json_writer_append_int32(jw, property_val));
    return AZ_OK;
}

/**********************************************
* Add a JSON key-value pair with string data
* e.g. "property_name" : "property_val (string)"
**********************************************/
az_result append_json_property_string(
    az_json_writer* jw,
    az_span         property_name_span,
    az_span         property_val_span)
{
    RETURN_ERR_IF_FAILED(az_json_writer_append_property_name(jw, property_name_span));
    RETURN_ERR_IF_FAILED(az_json_writer_append_string(jw, property_val_span));
    return AZ_OK;
}

/**********************************************
* Add JSON for writable property response with int32 data
* e.g. "property_name" : property_val_int32
**********************************************/
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
static az_result append_reported_property_response_int32(
    az_json_writer* jw,
    az_span         property_name_span,
    int32_t         property_val,
    int32_t         ack_code,
    int32_t         ack_version,
    az_span         ack_description_span)
{
    RETURN_ERR_IF_FAILED(az_iot_pnp_client_property_builder_begin_reported_status(&pnp_client,
                                                                                  jw,
                                                                                  property_name_span,
                                                                                  ack_code,
                                                                                  ack_version,
                                                                                  ack_description_span));

    RETURN_ERR_IF_FAILED(az_json_writer_append_int32(jw, property_val));
    RETURN_ERR_IF_FAILED(az_iot_pnp_client_property_builder_end_reported_status(&pnp_client, jw));
    return AZ_OK;
}
#endif
/**********************************************
* Build sensor telemetry JSON
**********************************************/
az_result build_sensor_telemetry_message(
    az_span* out_payload_span,
    int32_t  temperature,
    int32_t  light)
{
    az_json_writer jw;
    memset(&pnp_telemetry_payload_buffer, 0, sizeof(pnp_telemetry_payload_buffer));
    RETURN_ERR_IF_FAILED(start_json_object(&jw, AZ_SPAN_FROM_BUFFER(pnp_telemetry_payload_buffer)));
    RETURN_ERR_IF_FAILED(append_json_property_int32(&jw, telemetry_name_temperature_span, temperature));
    RETURN_ERR_IF_FAILED(append_json_property_int32(&jw, telemetry_name_light_span, light));
    RETURN_ERR_IF_FAILED(end_json_object(&jw));
    *out_payload_span = az_json_writer_get_bytes_used_in_destination(&jw);
    return AZ_OK;
}

/**********************************************
* Create JSON document for command error response
* e.g.
* {
*   "status":500,
*   "payload":{
*     "Status":"Unsupported Command"
*   }
* }
**********************************************/
static az_result build_command_error_response_payload(
    az_span  response_span,
    az_span  status_string_span,
    az_span* response_payload_span)
{
    az_json_writer jw;

    // Build the command response payload with error status
    RETURN_ERR_IF_FAILED(start_json_object(&jw, response_span));
    RETURN_ERR_IF_FAILED(append_json_property_string(&jw, command_status_span, status_string_span));
    RETURN_ERR_IF_FAILED(end_json_object(&jw));
    *response_payload_span = az_json_writer_get_bytes_used_in_destination(&jw);
    return AZ_OK;
}

/**********************************************
* Create JSON document for reboot command with status message
* e.g.
* {
*   "status":200,
*   "payload":{
*     "status":"Success",
*     "delay":100
*   }
* }
**********************************************/
static az_result build_reboot_command_resp_payload(
    az_span  response_span,
    int      reboot_delay,
    az_span* response_payload_span)
{
    az_json_writer jw;

    // Build the command response payload
    RETURN_ERR_IF_FAILED(start_json_object(&jw, response_span));
    RETURN_ERR_IF_FAILED(append_json_property_string(&jw, command_status_span, command_resp_success_span));
    RETURN_ERR_IF_FAILED(append_json_property_int32(&jw, command_reboot_delay_payload_span, reboot_delay));
    RETURN_ERR_IF_FAILED(end_json_object(&jw));
    *response_payload_span = az_json_writer_get_bytes_used_in_destination(&jw);
    return AZ_OK;
}

/**********************************************
* Create JSON document for command response with status message
* e.g.
* {
*   "status":200,
*   "payload":{
*     "status":"Success"
*   }
* }
**********************************************/
static az_result build_command_resp_payload(az_span response_span, az_span* response_payload_span)
{
    az_json_writer jw;

    // Build the command response payload
    RETURN_ERR_IF_FAILED(start_json_object(&jw, response_span));
    RETURN_ERR_IF_FAILED(append_json_property_string(&jw, command_status_span, command_resp_success_span));
    RETURN_ERR_IF_FAILED(end_json_object(&jw));
    *response_payload_span = az_json_writer_get_bytes_used_in_destination(&jw);
    return AZ_OK;
}

/**********************************************
* Append JSON key-value pairs for button name and button press count
* e.g.
* {
*   "button_event" :
*   {
*     "button_name" : "SW0",
*     "press_count" : 1
*   }
* }
**********************************************/
static az_result append_button_press_telemetry(
    az_json_writer* jw,
    az_span         button_name_span,
    int32_t         press_count)
{
    RETURN_ERR_IF_FAILED(az_json_writer_append_property_name(jw, event_name_button_event_span));
    RETURN_ERR_IF_FAILED(az_json_writer_append_begin_object(jw));
    RETURN_ERR_IF_FAILED(append_json_property_string(jw, event_name_button_name_span, button_name_span));
    RETURN_ERR_IF_FAILED(append_json_property_int32(jw, event_name_press_count_span, press_count));
    RETURN_ERR_IF_FAILED(az_json_writer_append_end_object(jw));
    return AZ_OK;
}


/**********************************************
* Check if button(s) was pressed.
* If yes, send event to IoT Hub.
**********************************************/
void check_button_status(void)
{
    az_span        button_event_payload_span = AZ_SPAN_FROM_BUFFER(button_event_buffer);
    az_json_writer jw;
    az_result      rc = AZ_OK;

    // save flags in case user pressed buttons very fast, and for error case.
    bool sw0_pressed = button_press_data.flag.sw0 == 1 ? true : false;
    bool sw1_pressed = button_press_data.flag.sw1 == 1 ? true : false;

    // clear the flags
    button_press_data.flag.as_uint16 = LED_FLAG_EMPTY;

    if (!sw0_pressed && !sw1_pressed)
    {
        return;
    }

    RETURN_IF_FAILED(start_json_object(&jw, button_event_payload_span));

    if (sw0_pressed)
    {
        debug_printInfo("AZURE: Button SW0 Count %lu", button_press_data.sw0_press_count);
        RETURN_IF_FAILED(append_button_press_telemetry(&jw, event_name_button_sw0_span, button_press_data.sw0_press_count));
    }

    if (sw1_pressed)
    {
        debug_printInfo("AZURE: Button SW1 Count %lu", button_press_data.sw1_press_count);
        RETURN_IF_FAILED(append_button_press_telemetry(&jw, event_name_button_sw1_span, button_press_data.sw1_press_count));
    }

    RETURN_IF_FAILED(end_json_object(&jw));

    button_event_payload_span = az_json_writer_get_bytes_used_in_destination(&jw);

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    rc = az_iot_pnp_client_telemetry_get_publish_topic(
        &pnp_client,
        AZ_SPAN_EMPTY,
#else
    rc = az_iot_hub_client_telemetry_get_publish_topic(
        &iothub_client,
#endif
        NULL,
        pnp_telemetry_topic_buffer,
        sizeof(pnp_telemetry_topic_buffer),
        NULL);

    if (az_result_succeeded(rc))
    {
        CLOUD_publishData((uint8_t*)pnp_telemetry_topic_buffer,
                          az_span_ptr(button_event_payload_span),
                          az_span_size(button_event_payload_span),
                          1);
    }
    return;
}

/**********************************************
* Read sensor data and send telemetry to cloud
**********************************************/
az_result send_telemetry_message(void)
{
    az_result rc = AZ_OK;
    az_span   telemetry_payload_span;

    int16_t temp  = APP_GetTempSensorValue();
    int32_t light = APP_GetLightSensorValue();

    debug_printGood("AZURE: Light: %d Temperature: %d", light, temp);

    RETURN_ERR_WITH_MESSAGE_IF_FAILED(
        build_sensor_telemetry_message(&telemetry_payload_span, temp, light),
        "Failed to build sensor telemetry JSON payload");

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    rc = az_iot_pnp_client_telemetry_get_publish_topic(&pnp_client,
                                                       AZ_SPAN_EMPTY,
#else
    rc = az_iot_hub_client_telemetry_get_publish_topic(&iothub_client,
#endif
                                                       NULL,
                                                       pnp_telemetry_topic_buffer,
                                                       sizeof(pnp_telemetry_topic_buffer),
                                                       NULL);

    if (az_result_succeeded(rc))
    {
        CLOUD_publishData((uint8_t*)pnp_telemetry_topic_buffer,
                          az_span_ptr(telemetry_payload_span),
                          az_span_size(telemetry_payload_span),
                          1);
    }
    return rc;
}

/**********************************************
* Check if LED status has changed or not.
* If any LED status has changed, update Device Twin
**********************************************/
void check_led_status(twin_properties_t* twin_properties)
{
    twin_properties_t* twin_properties_ptr;
    twin_properties_t  twin_properties_local;

    bool force_sync = false;   // force LED status synchronization if this request is coming from Twin Get

    if (twin_properties == NULL)
    {
        // twin data is not provided by the caller
        init_twin_data(&twin_properties_local);
        twin_properties_ptr = &twin_properties_local;
    }
    else
    {
        // twin data is provided by the caller through Get Twin
        twin_properties_ptr = twin_properties;
    }

    if (led_status.change_flag.as_uint16 == LED_FLAG_EMPTY && twin_properties_ptr->flag.is_initial_get == 0)
    {
        // no changes, nothing to update
        return;
    }

    debug_printInfo("AZURE: %s() LED Status 0x%x", __func__, led_status.change_flag.as_uint16);

    // if this is from Get Twin, update according to Desired Property
    force_sync = twin_properties_ptr->flag.is_initial_get == 1 ? true : false;

    if (led_status.change_flag.as_uint16 != LED_FLAG_EMPTY || force_sync)
    {
        if (led_status.change_flag.blue == 1 || force_sync)
        {
            if ((led_status.state_flag.blue & (LED_STATE_BLINK_SLOW | LED_STATE_BLINK_FAST)) != 0)
            {
                twin_properties_ptr->reported_led_blue = LED_TWIN_BLINK;
            }
            else if (led_status.state_flag.blue == LED_STATE_HOLD)
            {
                twin_properties_ptr->reported_led_blue = LED_TWIN_ON;
            }
            else
            {
                twin_properties_ptr->reported_led_blue = LED_TWIN_OFF;
            }
        }

        if (led_status.change_flag.green == 1 || force_sync)
        {
            if ((led_status.state_flag.green & (LED_STATE_BLINK_SLOW | LED_STATE_BLINK_FAST)) != 0)
            {
                twin_properties_ptr->reported_led_green = LED_TWIN_BLINK;
            }
            else if (led_status.state_flag.green == LED_STATE_HOLD)
            {
                twin_properties_ptr->reported_led_green = LED_TWIN_ON;
            }
            else
            {
                twin_properties_ptr->reported_led_green = LED_TWIN_OFF;
            }
        }

        if (led_status.change_flag.red == 1 || force_sync)
        {
            if ((led_status.state_flag.red & (LED_STATE_BLINK_SLOW | LED_STATE_BLINK_FAST)) != 0)
            {
                twin_properties_ptr->reported_led_red = LED_TWIN_BLINK;
            }
            else if (led_status.state_flag.red == LED_STATE_HOLD)
            {
                twin_properties_ptr->reported_led_red = LED_TWIN_ON;
            }
            else
            {
                twin_properties_ptr->reported_led_red = LED_TWIN_OFF;
            }
        }

        // clear flags
        led_status.change_flag.as_uint16 = LED_FLAG_EMPTY;

        // if this is from Get Twin, Device Twin code path will update reported properties
        if (!force_sync)
        {
            send_reported_property(twin_properties_ptr);
        }
    }
}

/**********************************************
* Process LED Update/Patch
* Sets Yellow LED based on Device Twin from IoT Hub
**********************************************/
void update_leds(
    twin_properties_t* twin_properties)
{
    // If desired properties are not set, send current LED states.
    // Otherwise, set LED state based on Desired property
    if (twin_properties->flag.yellow_led_found == 1 && twin_properties->desired_led_yellow != LED_TWIN_NO_CHANGE)
    {
        if (twin_properties->desired_led_yellow == LED_TWIN_ON)
        {
            LED_SetYellow(LED_STATE_HOLD);
        }
        else if (twin_properties->desired_led_yellow == LED_TWIN_OFF)
        {
            LED_SetYellow(LED_STATE_OFF);
        }
        else if (twin_properties->desired_led_yellow == LED_TWIN_BLINK)
        {
            LED_SetYellow(LED_STATE_BLINK_FAST);
        }
    }

    // If this is Twin Get, populate LED states for Red, Blue, Green LEDs
    if (twin_properties->flag.is_initial_get == 1)
    {
        check_led_status(twin_properties);
    }
}

/**********************************************
* Send the response of the command invocation
**********************************************/
static int send_command_response(
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    az_iot_pnp_client_command_request* request,
#else
    az_iot_hub_client_method_request* request,
#endif
    uint16_t status,
    az_span  response)
{
    // Get the response topic to publish the command response
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    int rc = az_iot_pnp_client_commands_response_get_publish_topic(
        &pnp_client,
#else
    int rc = az_iot_hub_client_methods_response_get_publish_topic(
        &iothub_client,
#endif
        request->request_id,
        status,
        command_topic_buffer,
        sizeof(command_topic_buffer),
        NULL);

    RETURN_ERR_WITH_MESSAGE_IF_FAILED(rc, "AZURE: Unable to get command response publish topic");

    debug_printInfo("AZURE: Command Status: %u", status);

    CLOUD_publishData((uint8_t*)command_topic_buffer,
                      az_span_ptr(response),
                      az_span_size(response),
                      1);

    return rc;
}

void reboot_task_callback(uintptr_t context)
{
    debug_printWarn("AZURE: Rebooting...");
    NVIC_SystemReset();
}

/**********************************************
*	Handle reboot command
**********************************************/
static az_result process_reboot_command(
    az_span   payload_span,
    az_span   response_span,
    az_span*  out_response_span,
    uint16_t* out_response_status)
{
    az_result      ret              = AZ_OK;
    char           reboot_delay[32] = {0};
    az_json_reader jr;
    *out_response_status = AZ_IOT_STATUS_SERVER_ERROR;

    if (az_span_size(payload_span) > 0)
    {
        debug_printInfo("AZURE: %s() : Payload %s", __func__, az_span_ptr(payload_span));

        RETURN_ERR_IF_FAILED(az_json_reader_init(&jr, payload_span, NULL));

        while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
        {
            if (jr.token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
            {
                if (az_json_token_is_text_equal(&jr.token, command_reboot_delay_payload_span))
                {
                    if (az_result_failed(ret = az_json_reader_next_token(&jr)))
                    {
                        debug_printError("AZURE: Error getting next token");
                        break;
                    }
                    else if (az_result_failed(ret = az_json_token_get_string(&jr.token, reboot_delay, sizeof(reboot_delay), NULL)))
                    {
                        debug_printError("AZURE: Error getting string");
                        break;
                    }

                    break;
                }
            }
            else if (az_result_failed(ret = az_json_reader_next_token(&jr)))
            {
                debug_printError("AZURE: Error getting next token");
                break;
            }
        }
    }
    else
    {
        debug_printError("AZURE: %s() : Payload Empty", __func__);
    }

    if (strlen(reboot_delay) == 0)
    {
        debug_printError("AZURE: Reboot Delay not found");

        ret = build_command_error_response_payload(response_span,
                                                   command_resp_empty_delay_payload_span,
                                                   out_response_span);

        *out_response_status = AZ_IOT_STATUS_BAD_REQUEST;
    }
    else if (reboot_delay[0] != 'P' || reboot_delay[1] != 'T' || reboot_delay[strlen(reboot_delay) - 1] != 'S')
    {
        debug_printError("AZURE: Reboot Delay wrong format");

        ret = build_command_error_response_payload(response_span,
                                                   command_resp_bad_payload_span,
                                                   out_response_span);

        *out_response_status = AZ_IOT_STATUS_BAD_REQUEST;
    }
    else
    {
        int reboot_delay_seconds = atoi((const char*)&reboot_delay[2]);

        RETURN_ERR_IF_FAILED(build_reboot_command_resp_payload(response_span,
                                                               reboot_delay_seconds,
                                                               out_response_span));

        *out_response_status = AZ_IOT_STATUS_ACCEPTED;

        debug_printInfo("AZURE: Scheduling reboot in %d sec", reboot_delay_seconds);

        reboot_task_handle = SYS_TIME_CallbackRegisterMS(reboot_task_callback,
                                                         0,
                                                         reboot_delay_seconds * 1000,
                                                         SYS_TIME_SINGLE);

        if (reboot_task_handle == SYS_TIME_HANDLE_INVALID)
        {
            debug_printError("AZURE: Failed to schedule reboot timer");
        }
    }

    return ret;
}

/**********************************************
 *	Handle send message command
 **********************************************/
static az_result process_sendMsg_command(
    az_span   payload_span,
    az_span   response_span,
    az_span*  out_response_span,
    uint16_t* out_response_status)
{
    az_result      ret           = AZ_OK;
    size_t         spanSize      = -1;
    char*          messageString = NULL;
    az_json_reader jr;

    *out_response_status = AZ_IOT_STATUS_SERVER_ERROR;

    if (az_span_size(payload_span) > 0)
    {
        debug_printInfo("AZURE: %s() : Payload %s", __func__, az_span_ptr(payload_span));

        RETURN_ERR_IF_FAILED(az_json_reader_init(&jr, payload_span, NULL));

        while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
        {
            if (jr.token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
            {
                if (az_json_token_is_text_equal(&jr.token, command_sendMsg_payload_span))
                {
                    if (az_result_failed(ret = az_json_reader_next_token(&jr)))
                    {
                        debug_printError("AZURE: Error getting next token");
                        break;
                    }
                    else
                    {
                        spanSize = (size_t)az_span_size(jr.token.slice);
                        break;
                    }

                    break;
                }
            } else if (az_result_failed(ret = az_json_reader_next_token(&jr)))
            {
                debug_printError("AZURE: Error getting next token");
                break;
            }
        }
    }
    else
    {
        debug_printError("AZURE: %s() : Payload Empty", __func__);
    }

    if (spanSize == -1)
    {
        debug_printError("AZURE: Message string not found");

        ret = build_command_error_response_payload(response_span,
                                                   command_resp_empty_sendMsg_payload_span,
                                                   out_response_span);

        *out_response_status = AZ_IOT_STATUS_BAD_REQUEST;
    }
    else if (spanSize > 1024)
    {
        debug_printError("AZURE: Message too big for TX Buffer %lu", spanSize);

        ret = build_command_error_response_payload(response_span,
                                                   command_resp_alloc_error_sendMsg_span,
                                                   out_response_span);

        *out_response_status = AZ_IOT_STATUS_BAD_REQUEST;
    }
    else if ((messageString = malloc(spanSize + 1)) == NULL)
    {
        debug_printError("AZURE: Failed to allocate memory for message string : Size %ld", spanSize);

        ret = build_command_error_response_payload(response_span,
                                                   command_resp_alloc_error_sendMsg_span,
                                                   out_response_span);

        *out_response_status = AZ_IOT_STATUS_BAD_REQUEST;
    }
    else
    {
        RETURN_ERR_IF_FAILED(az_json_token_get_string(&jr.token, messageString, spanSize + 1, NULL));

        debug_disable(true);
        SYS_CONSOLE_Message(0, messageString);
        debug_disable(false);
        RETURN_ERR_IF_FAILED(build_command_resp_payload(response_span, out_response_span));

        *out_response_status = AZ_IOT_STATUS_ACCEPTED;
    }

    if (messageString)
    {
        free(messageString);
    }

    return ret;
}

/**********************************************
* Process Command
**********************************************/
az_result process_direct_method_command(
    uint8_t* payload,
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    az_iot_pnp_client_command_request* command_request)
#else
    az_iot_hub_client_method_request* method_request)
#endif
{
    az_result rc                = AZ_OK;
    uint16_t  response_status   = AZ_IOT_STATUS_BAD_REQUEST;   // assume error
    az_span   command_resp_span = AZ_SPAN_FROM_BUFFER(command_resp_buffer);
    az_span   payload_span      = az_span_create_from_str((char*)payload);

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    debug_printInfo("AZURE: Processing Command '%s'", command_request->command_name);
#else
    debug_printInfo("AZURE: Processing Command '%s'", method_request->name);
#endif

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    if (az_span_is_content_equal(command_name_reboot_span, command_request->command_name))
#else
    if (az_span_is_content_equal(command_name_reboot_span, method_request->name))
#endif
    {
        rc = process_reboot_command(payload_span, command_resp_span, &command_resp_span, &response_status);

        if (az_result_failed(rc))
        {
            debug_printError("AZURE: Failed process_reboot_command, status 0x%08x", rc);
            if (az_span_size(command_resp_span) == 0)
            {
                // if response is empty, payload was not in the right format.
                if (az_result_failed(rc = build_command_error_response_payload(command_resp_span,
                                                                               command_resp_error_processing_span,
                                                                               &command_resp_span)))
                {
                    debug_printError("AZURE: Failed to build error response. (0x%08x)", rc);
                }
            }
        }
    }
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    else if (az_span_is_content_equal(command_name_sendMsg_span, command_request->command_name))
#else
    else if (az_span_is_content_equal(command_name_sendMsg_span, method_request->name))
#endif
    {
        rc = process_sendMsg_command(payload_span, command_resp_span, &command_resp_span, &response_status);

        if (az_result_failed(rc))
        {
            debug_printError("AZURE: Failed process_sendMsg_command, status 0x%08x", rc);
            if (az_span_size(command_resp_span) == 0)
            {
                // if response is empty, payload was not in the right format.
                if (az_result_failed(rc = build_command_error_response_payload(
                            command_resp_span, command_resp_error_processing_span, &command_resp_span)))
                {
                    debug_printError("AZURE: Failed to build error response. (0x%08x)", rc);
                }
            }
        }
    }
    else
    {
        rc = AZ_ERROR_NOT_SUPPORTED;
        // Unsupported command
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
        debug_printError("AZURE: Unsupported command received: %s.", az_span_ptr(command_request->command_name));
#else
        debug_printError("AZURE: Unsupported command received: %s.", az_span_ptr(method_request->name));
#endif
        // if response is empty, payload was not in the right format.
        if (az_result_failed(rc = build_command_error_response_payload(command_resp_span,
                                                                       command_resp_not_supported_span,
                                                                       &command_resp_span)))
        {
            debug_printError("AZURE: Failed to build error response. (0x%08x)", rc);
        }
    }

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    if ((rc = send_command_response(command_request, response_status, command_resp_span)) != 0)
#else
    if ((rc = send_command_response(method_request, response_status, command_resp_span)) != 0)
#endif
    {
        debug_printError("AZURE: Unable to send %d response, status %d", response_status, rc);
    }

    return rc;
}

#ifndef IOT_PLUG_AND_PLAY_MODEL_ID
// from az_iot_pnp_client_property.c
az_result json_child_token_move(az_json_reader* ref_jr, az_span property_name)
{
    do
    {
        if ((ref_jr->token.kind == AZ_JSON_TOKEN_PROPERTY_NAME) && az_json_token_is_text_equal(&(ref_jr->token), property_name))
        {
            RETURN_ERR_IF_FAILED(az_json_reader_next_token(ref_jr));

            return AZ_OK;
        }
        else if (ref_jr->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
        {
            if (az_result_failed(az_json_reader_skip_children(ref_jr)))
            {
                return AZ_ERROR_UNEXPECTED_CHAR;
            }
        }
        else if (ref_jr->token.kind == AZ_JSON_TOKEN_END_OBJECT)
        {
            return AZ_ERROR_ITEM_NOT_FOUND;
        }
    } while (az_result_succeeded(az_json_reader_next_token(ref_jr)));

    return AZ_ERROR_ITEM_NOT_FOUND;
}


static az_result get_twin_version(
    az_json_reader*                      ref_json_reader,
    az_iot_hub_client_twin_response_type response_type,
    int32_t*                             out_version)
{
    RETURN_ERR_IF_FAILED(az_json_reader_next_token(ref_json_reader));

    if (ref_json_reader->token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
        return AZ_ERROR_UNEXPECTED_CHAR;
    }

    RETURN_ERR_IF_FAILED(az_json_reader_next_token(ref_json_reader));

    if (response_type == AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET)
    {
        RETURN_ERR_IF_FAILED(json_child_token_move(ref_json_reader, iot_hub_property_desired));
        RETURN_ERR_IF_FAILED(az_json_reader_next_token(ref_json_reader));
    }

    RETURN_ERR_IF_FAILED(json_child_token_move(ref_json_reader, iot_hub_property_desired_version));
    RETURN_ERR_IF_FAILED(az_json_token_get_int32(&ref_json_reader->token, out_version));

    return AZ_OK;
}

static az_result get_twin_desired(
    az_json_reader*                      ref_json_reader,
    az_iot_hub_client_twin_response_type response_type)
{
    RETURN_ERR_IF_FAILED(az_json_reader_next_token(ref_json_reader));

    if (ref_json_reader->token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
        return AZ_ERROR_UNEXPECTED_CHAR;
    }

    RETURN_ERR_IF_FAILED(az_json_reader_next_token(ref_json_reader));

    if (response_type == AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET)
    {
        RETURN_ERR_IF_FAILED(json_child_token_move(ref_json_reader, iot_hub_property_desired));
    }

    return AZ_OK;
}

#endif
/**********************************************
* Parse Desired Property (Writable Property)
* Respond by updating Writable Property with IoT Plug and Play convention
* https://docs.microsoft.com/en-us/azure/iot-pnp/concepts-convention#writable-properties
* e.g.
* "reported": {
*   "telemetryInterval": {
*     "ac": 200,
*     "av": 13,
*     "ad": "Success",
*     "value": 60
* }
**********************************************/
az_result process_device_twin_property(
    uint8_t*           topic,
    uint8_t*           payload,
    twin_properties_t* twin_properties)
{
    az_result rc;
    az_span   property_topic_span;
    az_span   payload_span;

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    az_span component_name_span;

    az_iot_pnp_client_property_response property_response;
#else
    az_iot_hub_client_twin_response property_response;
#endif
    az_json_reader jr;

    property_topic_span = az_span_create(topic, strlen((char*)topic));
    payload_span        = az_span_create_from_str((char*)payload);

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    rc = az_iot_pnp_client_property_parse_received_topic(&pnp_client,
#else
    rc = az_iot_hub_client_twin_parse_received_topic(&iothub_client,
#endif
                                                         property_topic_span,
                                                         &property_response);

    if (az_result_succeeded(rc))
    {
        debug_printTrace("AZURE: Property Topic   : %s", az_span_ptr(property_topic_span));
        debug_printTrace("AZURE: Property Type    : %d", property_response.response_type);
        debug_printTrace("AZURE: Property Payload : %s", (char*)payload);
    }
    else
    {
        debug_printError("AZURE: Failed to parse property topic 0x%08x.", rc);
        debug_printError("AZURE: Topic: '%s' Payload: '%s'", (char*)topic, (char*)payload);
        return rc;
    }

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    if (property_response.response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET)
#else
    if (property_response.response_type == AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET)
#endif
    {
        if (az_span_is_content_equal_ignoring_case(property_response.request_id, twin_request_id_span))
        {
            debug_printInfo("AZURE: INITIAL GET Received");
            twin_properties->flag.is_initial_get = 1;
        }
        else
        {
            debug_printInfo("AZURE: Property GET Received");
        }
    }
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    else if (property_response.response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES)
#else
    else if (property_response.response_type == AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES)
#endif
    {
        debug_printInfo("AZURE: Property DESIRED Status %d Version %s",
                        property_response.status,
                        az_span_ptr(property_response.version));
    }
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    else if (property_response.response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_REPORTED_PROPERTIES)
#else
    else if (property_response.response_type == AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES)
#endif
    {
        if (!az_iot_status_succeeded(property_response.status))
        {
            debug_printInfo("AZURE: Property REPORTED Status %d Version %s",
                            property_response.status,
                            az_span_ptr(property_response.version));
        }

        // This is an acknowledgement from the service that it received our properties. No need to respond.
        return rc;
    }
    else
    {
        debug_printInfo("AZURE: Type %d Status %d ID %s Version %s",
                        property_response.response_type,
                        property_response.status,
                        az_span_ptr(property_response.request_id),
                        az_span_ptr(property_response.version));
    }

    rc = az_json_reader_init(&jr,
                             payload_span,
                             NULL);
    RETURN_ERR_WITH_MESSAGE_IF_FAILED(rc, "az_json_reader_init() for get version failed");

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID

    rc = az_iot_pnp_client_property_get_property_version(&pnp_client,
                                                         &jr,
                                                         property_response.response_type,
                                                         &twin_properties->version_num);

    RETURN_ERR_WITH_MESSAGE_IF_FAILED(rc, "az_iot_pnp_client_property_get_property_version() failed");
    twin_properties->flag.version_found = 1;

#else
    rc = get_twin_version(&jr,
                          property_response.response_type,
                          &twin_properties->version_num);

    RETURN_ERR_WITH_MESSAGE_IF_FAILED(rc, "get_twin_version() failed");
    twin_properties->flag.version_found = 1;

#endif

    rc = az_json_reader_init(&jr,
                             payload_span,
                             NULL);
    RETURN_ERR_WITH_MESSAGE_IF_FAILED(rc, "az_json_reader_init() failed");

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    while (az_result_succeeded(az_iot_pnp_client_property_get_next_component_property(
        &pnp_client,
        &jr,
        property_response.response_type,
        &component_name_span)))
    {
        if (az_json_token_is_text_equal(&jr.token, property_telemetry_interval_span))
        {
            uint32_t data;
            // found writable property to adjust telemetry interval
            RETURN_ERR_IF_FAILED(az_json_reader_next_token(&jr));
            RETURN_ERR_IF_FAILED(az_json_token_get_uint32(&jr.token, &data));
            twin_properties->flag.telemetry_interval_found = 1;
            telemetryInterval                              = data;
        }
        else if (az_json_token_is_text_equal(&jr.token, led_yellow_property_name_span))
        {
            // found writable property to control Yellow LED
            RETURN_ERR_IF_FAILED(az_json_reader_next_token(&jr));
            RETURN_ERR_IF_FAILED(az_json_token_get_int32(&jr.token,
                                                         &twin_properties->desired_led_yellow));
            twin_properties->flag.yellow_led_found = 1;
        }
        else
        {
            char   buffer[32];
            size_t spanSize = (size_t)az_span_size(jr.token.slice);
            size_t size     = sizeof(buffer) < spanSize ? sizeof(buffer) : spanSize;
            snprintf(buffer, size, "%s", az_span_ptr(jr.token.slice));

            debug_printWarn("AZURE: Received unknown property '%s'", buffer);
        }
        RETURN_ERR_IF_FAILED(az_json_reader_next_token(&jr));
    }
#else

    if (twin_properties->flag.is_initial_get == 1)
    {
        get_twin_desired(&jr, property_response.response_type);
    }
    else
    {
        RETURN_ERR_WITH_MESSAGE_IF_FAILED((az_json_reader_next_token(&jr)), "az_json_reader_next_token() failed.");
    }

    if (jr.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
        debug_printError(
            "`%.*s` property was not in the expected format. Token Kind %d",
            az_span_ptr(jr.token.slice),
            jr.token.kind);
        rc = AZ_ERROR_JSON_INVALID_STATE;
    }
    else
    {
        RETURN_ERR_WITH_MESSAGE_IF_FAILED((az_json_reader_next_token(&jr)), "az_json_reader_next_token() failed.");

        while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
        {
            if (jr.token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
            {
                if (az_json_token_is_text_equal(&jr.token, property_telemetry_interval_span))
                {
                    uint32_t data;
                    // found writable property to adjust telemetry interval
                    RETURN_ERR_IF_FAILED(az_json_reader_next_token(&jr));
                    RETURN_ERR_IF_FAILED(az_json_token_get_uint32(&jr.token, &data));
                    twin_properties->flag.telemetry_interval_found = 1;
                    telemetryInterval                              = data;
                }
                else if (az_json_token_is_text_equal(&jr.token, led_yellow_property_name_span))
                {
                    // found writable property to control Yellow LED
                    RETURN_ERR_IF_FAILED(az_json_reader_next_token(&jr));
                    RETURN_ERR_IF_FAILED(az_json_token_get_int32(&jr.token,
                                                                 &twin_properties->desired_led_yellow));
                    twin_properties->flag.yellow_led_found = 1;
                }
                else if (az_json_token_is_text_equal(&jr.token, iot_hub_property_desired_version))
                {
                    RETURN_ERR_IF_FAILED(az_json_reader_next_token(&jr));
                }
                else
                {
                    char   buffer[32];
                    size_t spanSize = (size_t)az_span_size(jr.token.slice) + 1;
                    size_t size     = sizeof(buffer) < spanSize ? sizeof(buffer) : spanSize;
                    snprintf(buffer, size, "%s", az_span_ptr(jr.token.slice));

                    debug_printWarn("AZURE: Received unknown property '%s'", buffer);
                    RETURN_ERR_IF_FAILED(az_json_reader_next_token(&jr));
                }
                RETURN_ERR_WITH_MESSAGE_IF_FAILED((az_json_reader_next_token(&jr)), "az_json_reader_next_token() failed.");
            }
        }
    }
#endif

    return rc;
}

int32_t get_led_value(uint16_t led_flag)
{
    int32_t led_property_value;

    if ((led_flag & (LED_STATE_BLINK_SLOW | LED_STATE_BLINK_FAST)) != 0)
    {
        led_property_value = LED_TWIN_BLINK;
    }
    else if (led_flag == LED_STATE_HOLD)
    {
        led_property_value = LED_TWIN_ON;
    }
    else
    {
        led_property_value = LED_TWIN_OFF;
    }

    return led_property_value;
}

/**********************************************
* Create AZ Span for Reported Property Request ID 
**********************************************/
static az_span get_request_id(void)
{
    az_span remainder;
    az_span out_span = az_span_create((uint8_t*)request_id_buffer,
                                      sizeof(request_id_buffer));

    az_result rc = az_span_u32toa(out_span,
                                  request_id_int++,
                                  &remainder);

    EXIT_WITH_MESSAGE_IF_FAILED(rc, "Failed to get request id");

    return az_span_slice(out_span, 0, az_span_size(out_span) - az_span_size(remainder));
}

/**********************************************
* Send Reported Property 
**********************************************/
az_result send_reported_property(
    twin_properties_t* twin_properties)
{
    az_result      rc;
    az_json_writer jw;
    az_span        identifier_span;
    int32_t        led_property_value;

    if (twin_properties->flag.as_uint16 == 0)
    {
        // Nothing to do.
        debug_printTrace("AZURE: No property update");
        return AZ_OK;
    }

    debug_printTrace("AZURE: Sending Property flag 0x%x", twin_properties->flag.as_uint16);

    // Clear buffer and initialize JSON Payload. This creates "{"
    memset(pnp_property_payload_buffer, 0, sizeof(pnp_property_payload_buffer));
    az_span payload_span = AZ_SPAN_FROM_BUFFER(pnp_property_payload_buffer);

    rc = start_json_object(&jw, payload_span);
    RETURN_ERR_WITH_MESSAGE_IF_FAILED(rc, "AZURE:Unable to initialize json writer for property PATCH");

    if (twin_properties->flag.telemetry_interval_found)
    {
        if (az_result_failed(
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
                rc = append_reported_property_response_int32(
                    &jw,
                    property_telemetry_interval_span,
                    telemetryInterval,
                    AZ_IOT_STATUS_OK,
                    twin_properties->version_num,
                    AZ_SPAN_FROM_STR("Success"))))
#else
                rc = append_json_property_int32(
                    &jw,
                    property_telemetry_interval_span,
                    telemetryInterval)))
#endif
        {
            debug_printError("AZURE: Unable to add property for telemetry interval, return code 0x%08x", rc);
            return rc;
        }
    }
    else if (twin_properties->flag.is_initial_get)
    {
        if (az_result_failed(

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
                rc = append_reported_property_response_int32(
                    &jw,
                    property_telemetry_interval_span,
                    telemetryInterval,
                    AZ_IOT_STATUS_OK,
                    1,
                    AZ_SPAN_FROM_STR("Success"))))
#else
                rc = append_json_property_int32(
                    &jw,
                    property_telemetry_interval_span,
                    telemetryInterval)))
#endif
        {
            debug_printError("AZURE: Unable to add property for telemetry interval, return code 0x%08x", rc);
            return rc;
        }
    }
    // Add Yellow LED to the reported property
    // Example with integer Enum
    if (twin_properties->desired_led_yellow != LED_TWIN_NO_CHANGE)
    {
        led_property_value = get_led_value(led_status.state_flag.yellow);

        if (az_result_failed(
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
                rc = append_reported_property_response_int32(
                    &jw,
                    led_yellow_property_name_span,
                    led_property_value,
                    AZ_IOT_STATUS_OK,
                    twin_properties->version_num,
                    AZ_SPAN_FROM_STR("Success"))))
#else
                rc = append_json_property_int32(
                    &jw,
                    led_yellow_property_name_span,
                    led_property_value)))
#endif
        {
            debug_printError("AZURE: Unable to add property for Yellow LED, return code 0x%08x", rc);
            return rc;
        }
    }
    else if (twin_properties->flag.is_initial_get)
    {
        led_property_value = get_led_value(led_status.state_flag.yellow);

        if (az_result_failed(

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
                rc = append_reported_property_response_int32(
                    &jw,
                    led_yellow_property_name_span,
                    led_property_value,
                    AZ_IOT_STATUS_OK,
                    1,
                    AZ_SPAN_FROM_STR("Success"))))
#else
                rc = append_json_property_int32(
                    &jw,
                    led_yellow_property_name_span,
                    led_property_value)))
#endif
        {
            debug_printError("AZURE: Unable to add property for Yellow LED, return code 0x%08x", rc);
            return rc;
        }
    }

    // Add Red LED
    // Example with String Enum
    if (twin_properties->flag.is_initial_get || twin_properties->reported_led_red != LED_TWIN_NO_CHANGE)
    {
        if (az_result_failed(
                rc = append_json_property_int32(
                    &jw,
                    led_red_property_name_span,
                    twin_properties->reported_led_red)))
        {
            debug_printError("AZURE: Unable to add property for Red LED, return code  0x%08x", rc);
            return rc;
        }
    }

    // Add Blue LED
    if (twin_properties->flag.is_initial_get || twin_properties->reported_led_blue != LED_TWIN_NO_CHANGE)
    {
        if (az_result_failed(
                rc = append_json_property_int32(
                    &jw,
                    led_blue_property_name_span,
                    twin_properties->reported_led_blue)))
        {
            debug_printError("AZURE: Unable to add property for Blue LED, return code  0x%08x", rc);
            return rc;
        }
    }

    // Add Green LED
    if (twin_properties->flag.is_initial_get || twin_properties->reported_led_green != LED_TWIN_NO_CHANGE)
    {
        if (az_result_failed(
                rc = append_json_property_int32(
                    &jw,
                    led_green_property_name_span,
                    twin_properties->reported_led_green)))
        {
            debug_printError("AZURE: Unable to add property for Green LED, return code  0x%08x", rc);
            return rc;
        }
    }

    // Add IP Address
    if (twin_properties->flag.is_initial_get || twin_properties->flag.ip_address_updated != 0)
    {
        if (az_result_failed(
                rc = append_json_property_string(
                    &jw,
                    ip_address_property_name_span,
                    az_span_create_from_str((char*)&twin_properties->ip_address))))
        {
            debug_printError("AZURE: Unable to add property for IP Address, return code  0x%08x", rc);
            return rc;
        }
    }

    if (twin_properties->flag.is_initial_get)
    {
        tstrM2mRev fwInfo;
        char firmwareString[18]; // 8bit + 8bit + 8bit + 16bit + 3 dots

        nm_get_firmware_full_info(&fwInfo);

        sprintf(firmwareString, "%u.%u.%u.%u", 
                    fwInfo.u8FirmwareMajor,
                    fwInfo.u8FirmwareMinor,
                    fwInfo.u8FirmwarePatch,
                    fwInfo.u16FirmwareSvnNum);

        if (az_result_failed(
                rc = append_json_property_string(
                    &jw,
                    fw_version_property_name_span,
                    az_span_create_from_str((char*)&firmwareString))))
        {
            debug_printError("AZURE: Unable to add property for Firmware Version, return code  0x%08x", rc);
            return rc;
        }
    }


    // Close JSON Payload (appends "}")
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    if (az_result_failed(rc = az_iot_pnp_client_property_builder_end_reported_status(&pnp_client, &jw)))
#else
    if (az_result_failed(rc = az_json_writer_append_end_object(&jw)))
#endif
    {
        debug_printError("AZURE: Unable to append end object, return code  0x%08x", rc);
        return rc;
    }

    az_span property_payload_span = az_json_writer_get_bytes_used_in_destination(&jw);

    // Publish the reported property payload to IoT Hub
    identifier_span = get_request_id();

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    rc = az_iot_pnp_client_property_patch_get_publish_topic(&pnp_client,
#else
    rc = az_iot_hub_client_twin_patch_get_publish_topic(&iothub_client,
#endif
                                                            identifier_span,
                                                            pnp_property_topic_buffer,
                                                            sizeof(pnp_property_topic_buffer),
                                                            NULL);
    RETURN_ERR_WITH_MESSAGE_IF_FAILED(rc, "AZURE:Failed to get property PATCH topic");

    // Send the reported property
    CLOUD_publishData((uint8_t*)pnp_property_topic_buffer,
                      az_span_ptr(property_payload_span),
                      az_span_size(property_payload_span),
                      1);

    return rc;
}
