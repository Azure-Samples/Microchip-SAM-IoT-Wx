// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include "mqtt/mqtt_core/mqtt_core.h"
#include "mqtt/mqtt_packetTransfer_interface.h"
#include "mqtt_packetPopulate.h"
#include "mqtt_packetPopulate.h"
#include "mqtt_iotprovisioning_packetPopulate.h"
#include "iot_config/IoT_Sensor_Node_config.h"
#include "azutil.h"
#include "debug_print.h"
#include "lib/basic/atca_basic.h"
#include "led.h"
#include "azure/iot/az_iot_provisioning_client.h"
#include "azure/core/az_span.h"

#ifdef CFG_MQTT_PROVISIONING_HOST
#define HALF_SECOND_MS 500L

/**
* @brief Provisioning polling interval.
*
* @details This is used for cases where the service does not supply a retry-after hint during the 
*          register and query operations.
*/
#ifndef PROVISIONING_POLLING_INTERVAL_S
    #define PROVISIONING_POLLING_INTERVAL_S (3U)
#endif

pf_MQTT_CLIENT pf_mqtt_iotprovisioning_client = {
    MQTT_CLIENT_iotprovisioning_publish,
    MQTT_CLIENT_iotprovisioning_receive,
    MQTT_CLIENT_iotprovisioning_connect,
    MQTT_CLIENT_iotprovisioning_subscribe,
    MQTT_CLIENT_iotprovisioning_connected,
    NULL};

extern const az_span device_model_id_span;
extern uint8_t       device_id_buffer[128 + 1];

/** \brief MQTT publish handler call back table.
 *
 * This callback table lists the callback function for to be called on reception
 * of a PUBLISH message for each topic which the application has subscribed to.
 * For each new topic which is subscribed to by the application, there needs to
 * be a corresponding publish handler.
 * E.g.: For a particular topic
 *       mchp/mySubscribedTopic/myDetailedPath
 *       Sample publish handler function  = void handlePublishMessage(uint8_t *topic, uint8_t *payload)
 */
extern publishReceptionHandler_t imqtt_publishReceiveCallBackTable[MAX_NUM_TOPICS_SUBSCRIBE];

uint8_t atca_dps_id_scope[11 + 1] = "0ne12345678";
char    hub_hostname_buffer[67 + 1];   // IoT Hub name 3 < 50, + azure-devices.net -> 67

az_iot_provisioning_client                   provisioning_client;
az_iot_provisioning_client_register_response dps_register_response;
char                                         mqtt_dps_topic_buffer[255];
az_span                                      register_payload_span;
char                                         register_payload_buffer[1024];
az_span                                      span_remainder;

static uint16_t        dps_retry_counter;
static SYS_TIME_HANDLE dps_retry_timer_handle = SYS_TIME_HANDLE_INVALID;
static void            dps_retry_task(uintptr_t context);

static SYS_TIME_HANDLE dps_assigning_timer_handle = SYS_TIME_HANDLE_INVALID;
static void            dps_assigning_task(uintptr_t context);

void MQTT_CLIENT_iotprovisioning_publish(uint8_t* topic, uint8_t* payload, uint16_t payload_len, int qos)
{
    debug_printWarn("  DPS: %s() not implemented", __FUNCTION__);
    return;
}

void MQTT_CLIENT_iotprovisioning_receive(uint8_t* data, uint16_t len)
{
    debug_printTrace("  DPS: received");
    MQTT_GetReceivedData(data, len);
}

void MQTT_CLIENT_iotprovisioning_connect(char* device_id)
{
    size_t mqtt_username_buffer_len;
    bool   bRet = false;   // assume failure

    debug_printInfo("  DPS: Sending MQTT CONNECT to %s", CFG_MQTT_PROVISIONING_HOST);

    LED_SetGreen(LED_STATE_BLINK_FAST);

    // Create a local span for Device ID, then copy into global span
    const az_span device_id_span_local = az_span_create_from_str(device_id);
    device_id_span                     = AZ_SPAN_FROM_BUFFER(device_id_buffer);
    az_span_copy(device_id_span, device_id_span_local);
    device_id_span = az_span_slice(device_id_span, 0, az_span_size(device_id_span_local));

    // Create a span for DPS endpoint
    const az_span global_device_endpoint_span = AZ_SPAN_LITERAL_FROM_STR(CFG_MQTT_PROVISIONING_HOST);

    // Create a span for ID Scope.
    // Read the ID Scope from the secure element (e.g. ATECC608A)
    atcab_read_bytes_zone(ATCA_ZONE_DATA, ATCA_SLOT_DPS_IDSCOPE, 0, atca_dps_id_scope, sizeof(atca_dps_id_scope));
    const az_span id_scope_span = az_span_create_from_str((char*)atca_dps_id_scope);

    debug_printGood("  DPS: ID Scope=%s from secure element", atca_dps_id_scope);

    // Sanity check ID Scope
    if (strlen((char*)atca_dps_id_scope) < 11)
    {
        debug_printError("  DPS: invalid IDScope");
    }
    // Initialize Provisioning Client
    else if (az_result_failed(az_iot_provisioning_client_init(&provisioning_client,
                                                              global_device_endpoint_span,
                                                              id_scope_span,
                                                              device_id_span,
                                                              NULL)))
    {
        debug_printError("  DPS: az_iot_provisioning_client_init failed");
    }
    // Get user name for MQTT connect
    else if (az_result_failed(az_iot_provisioning_client_get_user_name(&provisioning_client,
                                                                       mqtt_username_buffer,
                                                                       sizeof(mqtt_username_buffer),
                                                                       &mqtt_username_buffer_len)))
    {
        debug_printError("  DPS: az_iot_provisioning_client_get_user_name failed");
    }
    else
    {
        mqttConnectPacket cloudConnectPacket;
        memset(&cloudConnectPacket, 0, sizeof(mqttConnectPacket));
        cloudConnectPacket.connectVariableHeader.connectFlagsByte.cleanSession = 1;
        cloudConnectPacket.connectVariableHeader.keepAliveTimer                = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;
        cloudConnectPacket.clientID                                            = (uint8_t*)az_span_ptr(device_id_span);
        cloudConnectPacket.password                                            = NULL;
        cloudConnectPacket.passwordLength                                      = 0;
        cloudConnectPacket.username                                            = (uint8_t*)mqtt_username_buffer;
        cloudConnectPacket.usernameLength                                      = (uint16_t)mqtt_username_buffer_len;

        debug_printTrace("  DPS: ConnectPacket username(%d): %s", mqtt_username_buffer_len, mqtt_username_buffer);

        if ((bRet = MQTT_CreateConnectPacket(&cloudConnectPacket)) == false)
        {
            debug_printError("  DPS: Failed to create CONNECT packet");
        }
    }

    if (!bRet)
    {
        LED_SetRed(LED_STATE_BLINK_FAST);
    }
}

// Callback for DPS client register SUBSCRIBE
void dps_client_register(uint8_t* topic, uint8_t* payload)
{
    az_result rc;
    int topic_len = strlen((const char*)topic);
    int payload_len = strlen((const char*)payload);

    debug_printInfo("   DPS: %s()", __func__);

    if (az_result_failed(
            rc = az_iot_provisioning_client_parse_received_topic_and_payload(
                    &provisioning_client,
                    az_span_create(topic, topic_len),
                    az_span_create(payload, payload_len),
                    &dps_register_response)))
    {
        if ( rc != AZ_ERROR_IOT_TOPIC_NO_MATCH )
        {
            debug_printError("   DPS: az_iot_provisioning_client_parse_received_topic_and_payload fail:%d\n", (int)rc);
        }
        else
        {
            debug_printWarn("   DPS: ignoring unknown topic.\n");
        }
    }
    else if (az_iot_provisioning_client_operation_complete (dps_register_response.operation_status))
    {
        switch (dps_register_response.operation_status)
        {
            case AZ_IOT_PROVISIONING_STATUS_ASSIGNED:
                debug_printGood("   DPS: ASSIGNED");
                SYS_TIME_TimerDestroy(dps_retry_timer_handle);
                SYS_TIME_TimerDestroy(dps_assigning_timer_handle);

                az_span_to_str(hub_hostname_buffer, sizeof(hub_hostname_buffer), dps_register_response.registration_state.assigned_hub_hostname);
                hub_hostname = hub_hostname_buffer;
                LED_SetCloud(LED_INDICATOR_PENDING);
                pf_mqtt_iotprovisioning_client.MQTT_CLIENT_task_completed();
                break;

            case AZ_IOT_PROVISIONING_STATUS_FAILED:
                debug_printError("   DPS: FAILED with error %lu: TrackingID: [%.*s] \"%.*s\"",
                    (unsigned long)dps_register_response.registration_state.extended_error_code,
                    (size_t)az_span_size(dps_register_response.registration_state.error_tracking_id),
                    (char *)az_span_ptr(dps_register_response.registration_state.error_tracking_id),
                    (size_t)az_span_size(dps_register_response.registration_state.error_message),
                    (char *)az_span_ptr(dps_register_response.registration_state.error_message));

                LED_SetCloud(LED_INDICATOR_ERROR);
                break;

            case AZ_IOT_PROVISIONING_STATUS_DISABLED:
                debug_printError("   DPS: device is DISABLED");
                LED_SetCloud(LED_INDICATOR_ERROR);
                break;

            default:
                debug_printError("   DPS: unexpected status: %d\n", (int)dps_register_response.operation_status);
                LED_SetCloud(LED_INDICATOR_ERROR);
                break;
        }
    }
    else // Operation is not complete.
    {
        if (dps_register_response.retry_after_seconds == 0)
        {
            dps_register_response.retry_after_seconds = PROVISIONING_POLLING_INTERVAL_S;
        }

        debug_printInfo("   DPS: ASSIGNING");
        dps_assigning_timer_handle = SYS_TIME_CallbackRegisterMS(dps_assigning_task,
                                                            0,
                                                            1000 * dps_register_response.retry_after_seconds,
                                                            SYS_TIME_SINGLE);
    }
}

// Send DPS client register SUBSCRIBE
bool MQTT_CLIENT_iotprovisioning_subscribe()
{
    bool bRet = false;   // assume failure

    debug_printTrace("  DPS: Sending MQTT SUBSCRIBE");
    // Initialize SUBSCRIBE for DPS register
    mqttSubscribePacket cloudSubscribePacket = {0};

    cloudSubscribePacket.packetIdentifierLSB = 1;
    cloudSubscribePacket.packetIdentifierMSB = 0;

    cloudSubscribePacket.subscribePayload[0].topic        = (uint8_t*)AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC;
    cloudSubscribePacket.subscribePayload[0].topicLength  = sizeof(AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC) - 1;
    cloudSubscribePacket.subscribePayload[0].requestedQoS = 0;

    // Set up callback
    memset(imqtt_publishReceiveCallBackTable, 0, sizeof(imqtt_publishReceiveCallBackTable));
    imqtt_publishReceiveCallBackTable[0].topic                         = (uint8_t*)AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC;
    imqtt_publishReceiveCallBackTable[0].mqttHandlePublishDataCallBack = dps_client_register;
    MQTT_SetPublishReceptionHandlerTable(imqtt_publishReceiveCallBackTable);

    if ((bRet = MQTT_CreateSubscribePacket(&cloudSubscribePacket)) == false)
    {
        debug_printError("  DPS: Failed to create SUBSCRIBE packet");
        LED_SetCloud(LED_INDICATOR_ERROR);
    }

    return bRet;
}

void MQTT_CLIENT_iotprovisioning_connected()
{
    bool      bRet = false;   // assume failure
    az_result rc;

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    debug_printGood("  DPS: MQTT Connected.  Announcing DTMI '%s'", az_span_ptr(device_model_id_span));
#else
    debug_printGood("  DPS: MQTT Connected.");
#endif
    if (az_result_failed(rc = az_iot_provisioning_client_register_get_publish_topic(&provisioning_client,
                                                                                    mqtt_dps_topic_buffer,
                                                                                    sizeof(mqtt_dps_topic_buffer),
                                                                                    NULL)))
    {
        debug_printError("az_iot_provisioning_client_register_get_publish_topic failed. rc = 0x%0x", rc);
    }
    else
    {
        // Initialize PUBLISH to send model id
        mqttPublishPacket cloudPublishPacket = {0};

        cloudPublishPacket.publishHeaderFlags.duplicate = 0;
        cloudPublishPacket.publishHeaderFlags.qos       = 0;
        cloudPublishPacket.publishHeaderFlags.retain    = 0;

        cloudPublishPacket.topic = (uint8_t*)mqtt_dps_topic_buffer;

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID

        // Initalize Payload with IoT Plug and Play model id
        register_payload_span = az_span_create((uint8_t*)register_payload_buffer, sizeof(register_payload_buffer));
        span_remainder        = az_span_copy(register_payload_span, az_span_create_from_str("{\"payload\":{\"modelId\":\""));
        span_remainder        = az_span_copy(span_remainder, device_model_id_span);
        span_remainder        = az_span_copy(span_remainder, az_span_create_from_str("\"}}"));
        az_span_copy_u8(span_remainder, '\0');

        cloudPublishPacket.payload       = (uint8_t*)register_payload_buffer;
        cloudPublishPacket.payloadLength = strlen(register_payload_buffer);
#endif
        if ((bRet = MQTT_CreatePublishPacket(&cloudPublishPacket)) != true)
        {
            debug_printError("  DPS: MQTT PUBLISH failed");
        }
        else
        {
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
            debug_printInfo("  DPS: Sending MQTT PUBLISH payload '%s'", az_span_ptr(register_payload_span));
#else
            debug_printInfo("  DPS: Sending MQTT PUBLISH");
#endif

            // keep retrying connecting to DPS
            dps_retry_counter      = 0;
            dps_retry_timer_handle = SYS_TIME_CallbackRegisterMS(dps_retry_task, 0, HALF_SECOND_MS, SYS_TIME_PERIODIC);
        }
    }

    if (!bRet)
    {
        LED_SetCloud(LED_INDICATOR_ERROR);
    }
}


static void dps_assigning_task(uintptr_t context)
{
    bool bRet = false;   // assume failure

    if (az_result_failed(az_iot_provisioning_client_query_status_get_publish_topic(
            &provisioning_client,
            dps_register_response.operation_id,
            mqtt_dps_topic_buffer,
            sizeof(mqtt_dps_topic_buffer),
            NULL)))
    {
        debug_printError("  DPS: az_iot_provisioning_client_query_status_get_publish_topic failed");
    }
    else
    {
        mqttPublishPacket cloudPublishPacket;
        // Fixed header
        cloudPublishPacket.publishHeaderFlags.duplicate = 0;
        cloudPublishPacket.publishHeaderFlags.qos       = 0;
        cloudPublishPacket.publishHeaderFlags.retain    = 0;
        // Variable header
        cloudPublishPacket.topic = (uint8_t*)mqtt_dps_topic_buffer;

        // Payload
        cloudPublishPacket.payload       = NULL;
        cloudPublishPacket.payloadLength = 0;

        if ((bRet = MQTT_CreatePublishPacket(&cloudPublishPacket)) == false)
        {
            debug_printError("  DPS: MQTT PUBLISH for DPS Query Status failed");
        }
    }

    if (!bRet)
    {
        LED_SetCloud(LED_INDICATOR_ERROR);
    }

    return;
}

static void dps_retry_task(uintptr_t context)
{
    if (++dps_retry_counter % 240 > 0)   // retry every 2 mins
        return;

    MQTT_CLIENT_iotprovisioning_connect((char*)device_id_buffer);
    return;
}

#endif   // CFG_MQTT_PROVISIONING_HOST
