/*
    \file   mqtt_packetPopulate.h
    \brief  MQTT Packet Populate header file.
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


#ifndef MQTT_PACKET_POPULATE_H
#define MQTT_PACKET_POPULATE_H


#include <stdbool.h>
#include <stdint.h>
#include "azure/core/az_span.h"
#include "azure/iot/az_iot_pnp_client.h"

extern char*             hub_hostname;
extern uint8_t           device_id_buffer[128 + 1];
extern az_span           device_id_span;
extern az_iot_pnp_client pnp_client;
extern char              mqtt_username_buffer[203 + 1];

typedef struct
{
    void (*MQTT_CLIENT_publish)(uint8_t* topic, uint8_t* payload, uint16_t payload_len, int qos);
    void (*MQTT_CLIENT_receive)(uint8_t* data, uint16_t len);
    void (*MQTT_CLIENT_connect)(char* device_id);
    bool (*MQTT_CLIENT_subscribe)();
    void (*MQTT_CLIENT_connected)();
    void (*MQTT_CLIENT_task_completed)();
} pf_MQTT_CLIENT;

static const az_span twin_request_id_span = AZ_SPAN_LITERAL_FROM_STR("initial_get");

#endif /* MQTT_PACKET_POPULATE_H */
