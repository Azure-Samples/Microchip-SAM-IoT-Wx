/*
    \file   mqtt_packetPopulate.c
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
#include <ctype.h>
#include "mqtt_packetPopulate.h"
#include "iot_config/cloud_config.h"
#include "iot_config/IoT_Sensor_Node_config.h"

char* hub_hostname = CFG_MQTT_HUB_HOST;

uint8_t           device_id_buffer[128 + 1];   // Maximum number of characters in a device ID = 128
az_span           device_id_span;

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
az_iot_pnp_client pnp_client;
#else
az_iot_hub_client iothub_client;
#endif

/*
* MQTT User Name for DPS
* {idScope}/registrations/{registration_id}/api-version=2019-03-31
* idScope = 11 characters : e.g. 0ne01234567
* registration_id = Up to 128 characters
* 38 + 11 + 128 = 177
*
* MQTT User Name for IoT Hub
* {iothubhostname}/{device_id}/?api-version=2018-06-30
* iothubhostname =  <IoT Hub Name>.azure-devices.net
*   IoT Hub Name = up to 50 characters
* device_id = up to 128
* 25 + 50 + 128 = 203
*/
char mqtt_username_buffer[203 + 1];
