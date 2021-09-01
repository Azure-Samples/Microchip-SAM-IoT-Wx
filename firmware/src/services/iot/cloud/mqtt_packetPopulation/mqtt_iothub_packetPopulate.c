/*
	\file   mqtt_iothub_packetPopulate.c

	\brief  MQTT Packet Parameters source file.

	(c) 2018 Microchip Technology Inc. and its subsidiaries.

	Subject to your compliance with these terms, you may use Microchip software and any
	derivatives exclusively with Microchip products. It is your responsibility to comply with third party
	license terms applicable to your use of third party software (including open source software) that
	may accompany Microchip software.

	THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
	EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
	IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
	FOR A PARTICULAR PURPOSE.

	IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
	INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
	WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
	HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
	THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
	CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
	OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
	SOFTWARE.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include "mqtt/mqtt_core/mqtt_core.h"
#include "mqtt/mqtt_packetTransfer_interface.h"
#include "mqtt_packetPopulate.h"
#include "mqtt_iothub_packetPopulate.h"
#include "iot_config/IoT_Sensor_Node_config.h"
#include "azutil.h"
#include "debug_print.h"
#include "led.h"
#include "lib/basic/atca_basic.h"
#include "azure/iot/az_iot_pnp_client.h"
#include "azure/core/az_span.h"

pf_MQTT_CLIENT pf_mqtt_iothub_client = {
    MQTT_CLIENT_iothub_publish,
    MQTT_CLIENT_iothub_receive,
    MQTT_CLIENT_iothub_connect,
    MQTT_CLIENT_iothub_subscribe,
    MQTT_CLIENT_iothub_connected,
    NULL};

// Callback functions for IoT Hub SUBSCRIBE
extern void APP_ReceivedFromCloud_methods(uint8_t* topic, uint8_t* payload);
extern void APP_ReceivedFromCloud_twin(uint8_t* topic, uint8_t* payload);
extern void APP_ReceivedFromCloud_patch(uint8_t* topic, uint8_t* payload);

extern const az_span     device_model_id_span;
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
extern az_iot_pnp_client pnp_client;
#else
extern az_iot_hub_client iothub_client;
#endif

extern az_span           device_id_span;
extern char              mqtt_username_buffer[203 + 1];

static char mqtt_get_twin_topic_buffer[64];

static volatile uint16_t packet_identifier;

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
publishReceptionHandler_t imqtt_publishReceiveCallBackTable[MAX_NUM_TOPICS_SUBSCRIBE];

void MQTT_CLIENT_iothub_publish(uint8_t* topic, uint8_t* payload, uint16_t payload_len, int qos)
{
    uint16_t packet_id = 0;
    int      qos_value = 0;

    if (topic == NULL)
    {
        debug_printError("  HUB: %s() missing PUBLISH topic");
        return;
    }

    if (qos == 1)
    {
        qos_value = 1;
        packet_id = ++packet_identifier;
    }

    mqttPublishPacket cloudPublishPacket;
    // Fixed header
    cloudPublishPacket.publishHeaderFlags.duplicate = 0;
    cloudPublishPacket.publishHeaderFlags.qos       = qos_value;
    cloudPublishPacket.publishHeaderFlags.retain    = 0;

    if (qos_value == 1)
    {
        cloudPublishPacket.packetIdentifierLSB = packet_id & 0xff;
        cloudPublishPacket.packetIdentifierMSB = packet_id >> 8;
    }

    cloudPublishPacket.topic = topic;

    // Payload
    cloudPublishPacket.payload       = payload;
    cloudPublishPacket.payloadLength = payload_len;

    if (MQTT_CreatePublishPacket(&cloudPublishPacket) != true)
    {
        debug_printError("  HUB: MQTT_CLIENT_iothub_publish() failed");
    }

    return;
}

void MQTT_CLIENT_iothub_receive(uint8_t* data, uint16_t len)
{
    mqttHeaderFlags* mqtt_data = (mqttHeaderFlags*)data;

    if (mqtt_data->controlPacketType == PUBACK)
    {
#ifdef DEBUG_PUBACK
        mqttPubackPacket* mqtt_puback = (mqttPubackPacket*)data;
        uint16_t          identifier  = mqtt_puback->packetIdentifierLSB << 8 | mqtt_puback->packetIdentifierMSB;

        debug_printGood("  HUB: Received PUBACK for Packet ID %d", identifier);
#endif
    }
    MQTT_GetReceivedData(data, len);
}

void MQTT_CLIENT_iothub_puback_callback(mqttPubackPacket* data)
{
#ifdef DEBUG_PUBACK
    debug_printGood("  HUB: %s() Packet %d", __FUNCTION__, (uint16_t)(data->packetIdentifierMSB << 8 | data->packetIdentifierLSB));
#endif
    return;
}

void MQTT_CLIENT_iothub_connect(char* device_id)
{
    az_result     rc;
    size_t        mqtt_connect_username_len;
    const az_span hub_hostname_span    = az_span_create_from_str(hub_hostname);
    device_id_span                     = AZ_SPAN_FROM_BUFFER(device_id_buffer);

    debug_printInfo("  HUB: Sending MQTT CONNECT to '%s'", hub_hostname);

    LED_SetCloud(LED_INDICATOR_PENDING);

    const az_span device_id_span_local = az_span_create_from_str(device_id);

    az_span_copy(device_id_span, device_id_span_local);

    device_id_span = az_span_slice(device_id_span, 0, az_span_size(device_id_span_local));

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID

    rc = az_iot_pnp_client_init(&pnp_client,
                                hub_hostname_span,
                                device_id_span,
                                device_model_id_span,
                                NULL);
#else
    rc = az_iot_hub_client_init(&iothub_client,
                                hub_hostname_span,
                                device_id_span,
                                NULL);

#endif

    if (az_result_failed(rc))
    {
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
        debug_printError("  HUB: az_iot_pnp_client_init() failed. rc = 0x%x", rc);
#else
        debug_printError("  HUB: az_iot_hub_client_init() failed. rc = 0x%x", rc);
#endif
        return;
    }

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    rc = az_iot_pnp_client_get_user_name(&pnp_client,
                                         mqtt_username_buffer,
                                         sizeof(mqtt_username_buffer),
                                         &mqtt_connect_username_len);
#else
    rc = az_iot_hub_client_get_user_name(&iothub_client,
                                         mqtt_username_buffer,
                                         sizeof(mqtt_username_buffer),
                                         &mqtt_connect_username_len);
#endif

    if (az_result_failed(rc))
    {
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
        debug_printError("  HUB: az_iot_pnp_client_get_user_name() failed. rc = 0x%x", rc);
#else
        debug_printError("  HUB: az_iot_hub_client_get_user_name() failed. rc = 0x%x", rc);
#endif

        return;
    }

    mqttConnectPacket cloudConnectPacket;
    memset(&cloudConnectPacket, 0, sizeof(mqttConnectPacket));
    cloudConnectPacket.connectVariableHeader.connectFlagsByte.All = 0x20;
    cloudConnectPacket.connectVariableHeader.keepAliveTimer       = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

    cloudConnectPacket.clientID       = az_span_ptr(device_id_span);
    cloudConnectPacket.password       = NULL;
    cloudConnectPacket.passwordLength = 0;
    cloudConnectPacket.username       = (uint8_t*)mqtt_username_buffer;
    cloudConnectPacket.usernameLength = (uint16_t)mqtt_connect_username_len;

    if ((MQTT_CreateConnectPacket(&cloudConnectPacket)) == false)
    {
        debug_printError("  HUB: Failed to create CONNECT packet to IoT Hub");
        LED_SetCloud(LED_INDICATOR_ERROR);
    }
}

bool MQTT_CLIENT_iothub_subscribe()
{
    mqttSubscribePacket cloudSubscribePacket;
    bool                bRet = false;   // assume failure

    debug_printInfo("  HUB: Sending MQTT SUBSCRIBE to '%s'", hub_hostname);

    // Variable header
    cloudSubscribePacket.packetIdentifierLSB = 1;
    cloudSubscribePacket.packetIdentifierMSB = 0;

    cloudSubscribePacket.subscribePayload[0].topic        = (uint8_t*)AZ_IOT_PNP_CLIENT_COMMANDS_SUBSCRIBE_TOPIC;
    cloudSubscribePacket.subscribePayload[0].topicLength  = sizeof(AZ_IOT_PNP_CLIENT_COMMANDS_SUBSCRIBE_TOPIC) - 1;
    cloudSubscribePacket.subscribePayload[0].requestedQoS = 0;
    cloudSubscribePacket.subscribePayload[1].topic        = (uint8_t*)AZ_IOT_PNP_CLIENT_PROPERTY_PATCH_SUBSCRIBE_TOPIC;
    cloudSubscribePacket.subscribePayload[1].topicLength  = sizeof(AZ_IOT_PNP_CLIENT_PROPERTY_PATCH_SUBSCRIBE_TOPIC) - 1;
    cloudSubscribePacket.subscribePayload[1].requestedQoS = 0;
    cloudSubscribePacket.subscribePayload[2].topic        = (uint8_t*)AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_SUBSCRIBE_TOPIC;
    cloudSubscribePacket.subscribePayload[2].topicLength  = sizeof(AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_SUBSCRIBE_TOPIC) - 1;
    cloudSubscribePacket.subscribePayload[2].requestedQoS = 0;

    imqtt_publishReceiveCallBackTable[0].topic                         = (uint8_t*)AZ_IOT_PNP_CLIENT_COMMANDS_SUBSCRIBE_TOPIC;
    imqtt_publishReceiveCallBackTable[0].mqttHandlePublishDataCallBack = APP_ReceivedFromCloud_methods;
    imqtt_publishReceiveCallBackTable[1].topic                         = (uint8_t*)AZ_IOT_PNP_CLIENT_PROPERTY_PATCH_SUBSCRIBE_TOPIC;
    imqtt_publishReceiveCallBackTable[1].mqttHandlePublishDataCallBack = APP_ReceivedFromCloud_patch;
    imqtt_publishReceiveCallBackTable[2].topic                         = (uint8_t*)AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_SUBSCRIBE_TOPIC;
    imqtt_publishReceiveCallBackTable[2].mqttHandlePublishDataCallBack = APP_ReceivedFromCloud_twin;
    MQTT_SetPublishReceptionHandlerTable(imqtt_publishReceiveCallBackTable);

    if ((bRet = MQTT_CreateSubscribePacket(&cloudSubscribePacket)) == false)
    {
        debug_printError("  HUB: Failed to create SUBSCRIBE packet to IoT Hub");
        LED_SetCloud(LED_INDICATOR_ERROR);
    }

    return bRet;
}

void MQTT_CLIENT_iothub_connected()
{
    // get the current state of the device twin
    debug_printGood("  HUB: MQTT_CLIENT_iothub_connected()");

    MQTT_Set_Puback_callback(MQTT_CLIENT_iothub_puback_callback);

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    az_result rc = az_iot_pnp_client_property_document_get_publish_topic(&pnp_client,
                                                                         twin_request_id_span,
                                                                         mqtt_get_twin_topic_buffer,
                                                                         sizeof(mqtt_get_twin_topic_buffer),
                                                                         NULL);
#else
    az_result rc = az_iot_hub_client_twin_document_get_publish_topic(&iothub_client,
                                                                     twin_request_id_span,
                                                                     mqtt_get_twin_topic_buffer,
                                                                     sizeof(mqtt_get_twin_topic_buffer),
                                                                     NULL);
#endif


    if (az_result_failed(rc))
    {
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
        debug_printError("  HUB: az_iot_pnp_client_property_document_get_publish_topic failed. rc = 0x%x", rc);
#else
        debug_printError("  HUB: az_iot_hub_client_twin_document_get_publish_topic failed. rc = 0x%x", rc);
#endif
        return;
    }

    mqttPublishPacket cloudPublishPacket;
    // Fixed header
    cloudPublishPacket.publishHeaderFlags.duplicate = 0;
    cloudPublishPacket.publishHeaderFlags.qos       = 0;
    cloudPublishPacket.publishHeaderFlags.retain    = 0;
    // Variable header
    cloudPublishPacket.topic = (uint8_t*)mqtt_get_twin_topic_buffer;

    // Payload
    cloudPublishPacket.payload       = NULL;
    cloudPublishPacket.payloadLength = 0;

    if (MQTT_CreatePublishPacket(&cloudPublishPacket) != true)
    {
        debug_printError("  HUB: PUBLISH failed");
        LED_SetCloud(LED_INDICATOR_ERROR);
    }

    pf_mqtt_iothub_client.MQTT_CLIENT_task_completed();
}
