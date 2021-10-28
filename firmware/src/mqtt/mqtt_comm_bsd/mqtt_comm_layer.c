/*
    \file   mqtt_comm_layer.c

    \brief  MQTT Communication layer source file.

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

#include <string.h>
#include <stdio.h>
#include "mqtt_comm_layer.h"
#include "../../iot_config/IoT_Sensor_Node_config.h"
#include "../mqtt_core/mqtt_core.h"
#include "../../services/iot/cloud/bsd_adapter/bsdWINC.h"
#include "debug_print.h"

// MQTT Tx buffer size: (1KB + 1) bytes payload to support 1024-byte string with NULL char, 35 bytes for CONNECT packet size (mqttConnectPacket), 50 bytes for topic
#define TX_BUFF_SIZE         ((1024 + 1) + 35 + 50)/*400*/ 
// MQTT Rx buffer size: How large does this really need to be?  Original setting of 2096 bytes seems to be way overkill...1KB seems to be the minimum required
#define RX_BUFF_SIZE         1024/*2096*/
#define USER_LENGTH          0
#define MQTT_KEEP_ALIVE_TIME 120

static mqttContext mqttConn;
static uint8_t*    mqttTxBuff = NULL;
static uint8_t    mqttRxBuff[RX_BUFF_SIZE];
static int8_t      mqqtSocket = -1;

void MQTT_ClientInitialize(void)
{
    MQTT_initialiseState();

    if (mqttTxBuff == NULL)
    {
        mqttTxBuff = malloc(TX_BUFF_SIZE);
        if (mqttTxBuff == NULL)
        {
            debug_printError(" MQTT: TX Buffer fail malloc");
            return;
        }
    }


    memset(mqttTxBuff, 0, sizeof(TX_BUFF_SIZE));
    memset(mqttRxBuff, 0, sizeof(RX_BUFF_SIZE));
    mqttConn.mqttDataExchangeBuffers.txbuff.start           = mqttTxBuff;
    mqttConn.mqttDataExchangeBuffers.txbuff.bufferLength    = TX_BUFF_SIZE;
    mqttConn.mqttDataExchangeBuffers.txbuff.currentLocation = mqttConn.mqttDataExchangeBuffers.txbuff.start;
    mqttConn.mqttDataExchangeBuffers.txbuff.dataLength      = 0;
    mqttConn.mqttDataExchangeBuffers.rxbuff.start           = mqttRxBuff;
    mqttConn.mqttDataExchangeBuffers.rxbuff.bufferLength    = RX_BUFF_SIZE;
    mqttConn.mqttDataExchangeBuffers.rxbuff.currentLocation = mqttConn.mqttDataExchangeBuffers.rxbuff.start;
    mqttConn.mqttDataExchangeBuffers.rxbuff.dataLength      = 0;

    mqttConn.tcpClientSocket = &mqqtSocket;
}

mqttContext* MQTT_GetClientConnectionInfo()
{
    return &mqttConn;
}


bool MQTT_Send(mqttContext* connectionPtr)
{
    bool ret = false;
    int  sendRet;
    if ((sendRet = BSD_send(*connectionPtr->tcpClientSocket, connectionPtr->mqttDataExchangeBuffers.txbuff.start, connectionPtr->mqttDataExchangeBuffers.txbuff.dataLength, 0)) > BSD_SUCCESS)
    {
        ret = true;
    }

    //debug_print(" MQTT: sendresult (%d)", sendRet);
    return ret;
}

bool MQTT_Close(mqttContext* connectionPtr)
{
    debug_printGood(" MQTT: MQTT Close");
    bool ret = false;
    if (BSD_close(*connectionPtr->tcpClientSocket) == BSD_SUCCESS)
    {
        ret = true;
    }
    return ret;
}

void MQTT_GetReceivedData(uint8_t* pData, uint16_t len)
{
    if (MQTT_ExchangeBufferInit(&mqttConn.mqttDataExchangeBuffers.rxbuff))
    {
        MQTT_ExchangeBufferWrite(&mqttConn.mqttDataExchangeBuffers.rxbuff, pData, len);
    }
}
