/*
    \file   cloud_service.c

    \brief  Cloud Service Abstraction Layer

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

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../../../iot_config/cloud_config.h"
#include "cloud_service.h"
#include "../../../iot_config/IoT_Sensor_Node_config.h"
#include "crypto_client/crypto_client.h"
#include "crypto_client/cryptoauthlib_main.h"
#include "debug_print.h"
#include "m2m_wifi.h"
#include "../../../app.h"
#include "bsd_adapter/bsdWINC.h"
#include "socket.h"
#include "../cloud/mqtt_packetPopulation/mqtt_packetPopulate.h"
#include "../../../mqtt/mqtt_core/mqtt_core.h"
#include "wifi_service.h"
#include "../../../credentials_storage/credentials_storage.h"
#include "../../../mqtt/mqtt_packetTransfer_interface.h"
#include "definitions.h"
#include "iot_config/mqtt_config.h"
#include "led.h"
#include "../../../config/SAMD21_WG_IOT/driver/winc/include/drv/driver/m2m_ssl.h"

#define UNIX_OFFSET 946684800

static bool cloudInitialized = false;
static bool waitingForMQTT   = false;

pf_MQTT_CLIENT*   pf_mqtt_client;
char*             mqtt_host;
volatile uint32_t mqttHostIP;
volatile uint32_t dnsRetryCount = 0;
char              mqttSubscribeTopic[TOPIC_SIZE];

static int8_t  connectMQTTSocket(void);
static void    connectMQTT();

bool sendSubscribe = true;
#define CLOUD_TASK_INTERVAL_MS      500L
#define CLOUD_MQTT_TIMEOUT_COUNT_MS 30000L   // 30 seconds max allowed to establish a connection
#define CLOUD_RESET_TIMEOUT_MS      2000L    // 2 seconds
#define DNS_RETRY_COUNT_MS          10000L / CLOUD_TASK_INTERVAL_MS
#define WIFI_CONNECT_TIMEOUT_MS     5000L   // 5 seconds

SYS_TIME_HANDLE cloudResetTaskHandle  = SYS_TIME_HANDLE_INVALID;
SYS_TIME_HANDLE mqttTimeoutTaskHandle = SYS_TIME_HANDLE_INVALID;
SYS_TIME_HANDLE wifiTimeoutTaskHandle = SYS_TIME_HANDLE_INVALID;

volatile bool mqttTimeoutTaskTmrExpired = false;
volatile bool cloudResetTaskTmrExpired  = false;
volatile bool wifiTimeoutTaskTmrExpired = false;

void mqttTimeoutTask(void);
void cloudResetTask(void);
void wifiTimeoutTask(void);

/** \brief MQTT publish handler call back table.
 *
 * This callback table lists the callback function for to be called on reception 
 * of a PUBLISH message for each topic which the application has subscribed to.
 * For each new topic which is subscribed to by the application, there needs to 
 * be a corresponding publish handler.
 * E.g.: For a particular topic 
 *       mchp/mySubscribedTopic/myDetailedPath
 *       Sample publish handler function  = void handlePublishMessage(uint8_t *topic, uint8_t *payload)
 * 
 */

packetReceptionHandler_t cloud_packetReceiveCallBackTable[CLOUD_PACKET_RECV_TABLE_SIZE];

static char* ateccsn = NULL;

void NETWORK_wifiSslCallback(uint8_t u8MsgType, void* pvMsg)
{
    switch (u8MsgType)
    {
        case M2M_SSL_REQ_ECC: {
            tstrEccReqInfo* ecc_request = (tstrEccReqInfo*)pvMsg;
            CRYPTO_CLIENT_processEccRequest(ecc_request);

            break;
        }

        case M2M_SSL_RESP_SET_CS_LIST: {
            tstrSslSetActiveCsList* pstrCsList = (tstrSslSetActiveCsList*)pvMsg;
            debug_printInfo("CLOUD: ActiveCS bitmap:%04x", pstrCsList->u32CsBMP);

            break;
        }

        default:
            break;
    }
}

socketState_t getSocketState()
{
    mqttContext*  context     = MQTT_GetClientConnectionInfo();
    socketState_t socketState = BSD_GetSocketState(*context->tcpClientSocket);

    return socketState;
}

void CLOUD_setdeviceId(char* id)
{
    ateccsn = id;
}

//
// Callbacks for timers
//
void mqttTimeoutTaskcb(uintptr_t context)
{
    mqttTimeoutTaskTmrExpired = true;
}

void wifiTimeoutTaskcb(uintptr_t context)
{
    wifiTimeoutTaskTmrExpired = true;
}

void cloudResetTaskcb(uintptr_t context)
{
    cloudResetTaskTmrExpired = true;
}

//
// Start reset.
//
void CLOUD_reset(void)
{
    debug_printInfo("CLOUD: Resetting cloud connection");
    cloudInitialized = false;
    CLOUD_disconnect();
}

//
// Reset request timer fires to start initialization.
//
void cloudResetTask(void)
{
    debug_printInfo("CLOUD: Reset task");
    cloudInitialized = reInit();
}

//
// MQTT connection request timed out.
//
void mqttTimeoutTask(void)
{
    if (shared_networking_params.haveMqttConnection == 0)
    {
        debug_printWarn("CLOUD: MQTT Connection Timeout");
        CLOUD_reset();
        waitingForMQTT = false;
    }
}

//
// WiFi connection request timed out.
//
void wifiTimeoutTask(void)
{
    // To Do : We should cancel timer by adding callback.
    if ((shared_networking_params.haveAPConnection == 0 && shared_networking_params.haveIpAddress == 0) ||
        shared_networking_params.haveERROR == 1)
    {
        LED_SetWiFi(LED_INDICATOR_ERROR);
        debug_printWarn("CLOUD: WiFi Connection Timeout");
        CLOUD_reset();
    }
}

//
// Called by App to set DPS/IoTHub MQTT.
//
void CLOUD_init_host(char* host, char* attDeviceID, pf_MQTT_CLIENT* pf_table)
{
    mqtt_host                           = host;
    mqttHostIP                          = 0;
    shared_networking_params.haveHostIp = 1;
    pf_mqtt_client                      = pf_table;
    CLOUD_setdeviceId(attDeviceID);
    MQTT_Set_Puback_callback(NULL);
}

//
// Initiates Socket Connection
//
static int8_t connectMQTTSocket(void)
{
    int8_t        ret = false;
    int8_t        sslInit;
    socketState_t socketState = getSocketState();

    debug_printInfo("CLOUD: Connecting Socket to '%s'", mqtt_host);

    sslInit = m2m_ssl_init(NETWORK_wifiSslCallback);
    if (sslInit != M2M_SUCCESS)
    {
        debug_printInfo("CLOUD: WiFi SSL Initialization failed");
    }
    else if (mqttHostIP == 0)
    {
        debug_printError("CLOUD: Need MQTT Host IP");
    }
    else if (socketState != SOCKET_CLOSED)
    {
        debug_printWarn("CLOUD: Socket State is not Closed.  State = %d", socketState);
    }
    else
    {
        struct bsd_sockaddr_in addr;

        addr.sin_family      = PF_INET;
        addr.sin_port        = BSD_htons(CFG_MQTT_PORT);
        addr.sin_addr.s_addr = mqttHostIP;

        mqttContext* context = MQTT_GetClientConnectionInfo();

        debug_printInfo("CLOUD: Configuring SSL SNI to connect to '%lu.%lu.%lu.%lu'",
                        (0x0FF & (mqttHostIP)),
                        (0x0FF & (mqttHostIP >> 8)),
                        (0x0FF & (mqttHostIP >> 16)),
                        (0x0FF & (mqttHostIP >> 24)));

        ret = BSD_setsockopt(*context->tcpClientSocket,
                             SOL_SSL_SOCKET,
                             SO_SSL_SNI,
                             mqtt_host,
                             strlen(mqtt_host));

        if (ret == BSD_SUCCESS)
        {
            int optVal = 1;

            ret = BSD_setsockopt(*context->tcpClientSocket,
                                 SOL_SSL_SOCKET,
                                 SO_SSL_ENABLE_SNI_VALIDATION,
                                 &optVal,
                                 sizeof(optVal));
        }

        if (ret == BSD_SUCCESS)
        {
            ret = BSD_connect(*context->tcpClientSocket,
                              (struct bsd_sockaddr*)&addr,
                              sizeof(struct bsd_sockaddr_in));
        }
        else
        {
            debug_printError("CLOUD: Socket connect failed");
            LED_SetRed(LED_STATE_BLINK_SLOW);
            shared_networking_params.haveERROR = 1;
            BSD_close(*context->tcpClientSocket);
        }
    }
    return ret;
}

//
// Initiates MQTT connection
//
static void connectMQTT()
{
    time_t currentTime;

    struct tm sys_time;

    RTC_RTCCTimeGet(&sys_time);
    currentTime = mktime(&sys_time);

    debug_printTrace("CLOUD: Sending MQTT CONNECT at %s", ctime(&currentTime));

    if (currentTime > 0)
    {
        pf_mqtt_client->MQTT_CLIENT_connect(ateccsn);
    }

    waitingForMQTT = true;
    debug_printInfo("CLOUD: Starting MQTT Timeout");
    mqttTimeoutTaskHandle = SYS_TIME_CallbackRegisterMS(mqttTimeoutTaskcb, 0, CLOUD_MQTT_TIMEOUT_COUNT_MS, SYS_TIME_SINGLE);

    // MQTT SUBSCRIBE packet will be sent after the MQTT connection is established.
    sendSubscribe = true;
}

//
// Calls MQTT SUBSCRIBE
//
void CLOUD_subscribe(void)
{
    if (shared_networking_params.haveMqttConnection != 1)
    {
        debug_printError("CLOUD: MQTT not connected");
        return;
    }

    debug_printTrace("CLOUD: Sending MQTT SUBSCRIBE");
    if (pf_mqtt_client->MQTT_CLIENT_subscribe() == true)
    {
        sendSubscribe = false;
    }
}

//
// Initiates MQTT DISCONNECT
// This forces a reconnect
//
void CLOUD_disconnect(void)
{
    if (MQTT_GetConnectionState() == CONNECTED)
    {
        debug_printWarn("CLOUD: Sending MQTT DISCONNECT");
        MQTT_Disconnect(MQTT_GetClientConnectionInfo());
    }
}

// Todo: This declaration supports the hack below
packetReceptionHandler_t* getSocketInfo(uint8_t sock);


void CLOUD_task(void)
{
    mqttContext*  mqttConnnectionInfo = MQTT_GetClientConnectionInfo();
    socketState_t socketState         = BSD_GetSocketState(*mqttConnnectionInfo->tcpClientSocket);

    switch (socketState)
    {
        case NOT_A_SOCKET:   // 0
        {
            if (!cloudInitialized)
            {
                if (shared_networking_params.cloudInitPending != 1)
                {
                    shared_networking_params.cloudInitPending = 1;
                    // Start initialization
                    debug_printInfo("CLOUD: Cloud Reset timer start with %d ms", CLOUD_RESET_TIMEOUT_MS);
                    cloudResetTaskHandle = SYS_TIME_CallbackRegisterMS(cloudResetTaskcb, 0, CLOUD_RESET_TIMEOUT_MS, SYS_TIME_SINGLE);
                }
            }
            else if (shared_networking_params.haveAPConnection == 0)
            {
                // No network yet.
                debug_printTrace("CLOUD: Waiting for WiFi AP Connection");
            }
            else if (shared_networking_params.haveIpAddress == 0)
            {
                // No IP yet.
                debug_printInfo("CLOUD: Waiting for DHCP IP Address");
            }
            //else if (shared_networking_params.haveHostIp == 0)
            else if (shared_networking_params.haveHostIp == 0)
            {
                // Need IP Address of MQTT Host to connect socket.
                if (dnsRetryCount > 0)
                {
                    // still waiting for DNS look up
                    dnsRetryCount--;
                    break;
                }
                else
                {
                    // send request to get Host IP
                    debug_printInfo("CLOUD: Getting IP for %s", mqtt_host);
                    if (gethostbyname((char*)mqtt_host) != M2M_SUCCESS)
                    {
                        debug_printError("CLOUD: gethostbyname failed");
                    }
                    else
                    {
                        dnsRetryCount = DNS_RETRY_COUNT_MS;
                    }
                }
            }
            else if (MQTT_GetConnectionState() == CONNECTED)
            {
                CLOUD_reset();
            }
            else
            {
                // Ready to connect socket
                assert(shared_networking_params.haveHostIp == 1);

                *mqttConnnectionInfo->tcpClientSocket = BSD_socket(PF_INET, BSD_SOCK_STREAM, 1);   // WINC_TLS

                if (*mqttConnnectionInfo->tcpClientSocket >= 0)
                {
                    packetReceptionHandler_t* sockInfo = getSocketInfo(*mqttConnnectionInfo->tcpClientSocket);
                    if (sockInfo != NULL)
                    {
                        sockInfo->socketState = SOCKET_CLOSED;
                    }
                    else
                    {
                        debug_printWarn("CLOUD: Socket Info Null");
                    }
                }
            }
            break;
        }
        case SOCKET_CLOSED:   // 1
        {
            // Connect to socket
            if (connectMQTTSocket() != BSD_SUCCESS)
            {
                debug_printError(" CLOUD: Failed to connect socket");
            }
            break;
        }
        case SOCKET_IN_PROGRESS:   //2
            break;

        case SOCKET_CONNECTED:   // 3
        {

            mqttCurrentState mqttState = MQTT_GetConnectionState();

            // Socket is connected.
            if (mqttState == DISCONNECTED)
            {
                // Start MQTT CONNECT
                connectMQTT();
            }
            else
            {
                // Process incoming
                mqttState = MQTT_ReceptionHandler(mqttConnnectionInfo);
                //debug_printWarn("CLOUD: MQTT Reception %d", mqttState);

                // Process outgoing
                mqttState = MQTT_TransmissionHandler(mqttConnnectionInfo);
                //debug_printWarn("CLOUD: MQTT Transmission %d", mqttState);

                // Todo: We already processed the data in place using PEEK, this just flushes the buffer
                BSD_recv(*MQTT_GetClientConnectionInfo()->tcpClientSocket,
                         MQTT_GetClientConnectionInfo()->mqttDataExchangeBuffers.rxbuff.start,
                         MQTT_GetClientConnectionInfo()->mqttDataExchangeBuffers.rxbuff.bufferLength,
                         0);
            }

            if (mqttState == CONNECTED)
            {
                waitingForMQTT                              = false;
                shared_networking_params.haveMqttConnection = 1;

                if (mqttTimeoutTaskHandle != SYS_TIME_HANDLE_INVALID)
                {
                    SYS_TIME_TimerStop(mqttTimeoutTaskHandle);
                    SYS_TIME_TimerDestroy(mqttTimeoutTaskHandle);
                    mqttTimeoutTaskHandle = SYS_TIME_HANDLE_INVALID;

                }

                if (cloudResetTaskHandle != SYS_TIME_HANDLE_INVALID)
                {
                    SYS_TIME_TimerStop(cloudResetTaskHandle);
                    SYS_TIME_TimerDestroy(cloudResetTaskHandle);
                    cloudResetTaskHandle = SYS_TIME_HANDLE_INVALID;
                }

                if (sendSubscribe == true)
                {
                    // Send MQTT SUBSCRIBE
                    CLOUD_subscribe();
                }
            }
            break;
        }
        case SOCKET_CLOSING:
            break;
    }
}

bool CLOUD_isConnected(void)
{
    if (MQTT_GetConnectionState() == CONNECTED)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void CLOUD_publishData(uint8_t* topic, uint8_t* payload, uint16_t payload_len, int qos)
{
    pf_mqtt_client->MQTT_CLIENT_publish(topic, payload, payload_len, qos);
}

void dnsHandler(uint8_t* domainName, uint32_t serverIP)
{
    if (serverIP != 0)
    {
        dnsRetryCount                       = 0;
        shared_networking_params.haveHostIp = 1;
        mqttHostIP                          = serverIP;

        debug_printGood(" WIFI: mqttHostIP '%lu.%lu.%lu.%lu'",
                        (0x0FF & (serverIP)),
                        (0x0FF & (serverIP >> 8)),
                        (0x0FF & (serverIP >> 16)),
                        (0x0FF & (serverIP >> 24)));
    }
}

uint8_t reInit(void)
{
    debug_printInfo("CLOUD: reInit()");

    shared_networking_params.allBits = 0;
    waitingForMQTT                   = false;
    uint8_t wifi_creds;

    // Clear LEDs
    LED_SetWiFi(LED_INDICATOR_OFF);
    LED_SetCloud(LED_INDICATOR_OFF);
    LED_SetRed(LED_STATE_OFF);
    LED_SetYellow(LED_STATE_OFF);

    socketDeinit();
    socketInit();

    registerSocketCallback(BSD_SocketHandler, dnsHandler);

    MQTT_ClientInitialize();

    // Set callback for MQTT Message receive
    memset(&cloud_packetReceiveCallBackTable, 0, sizeof(cloud_packetReceiveCallBackTable));
    cloud_packetReceiveCallBackTable[0].socket       = MQTT_GetClientConnectionInfo()->tcpClientSocket;
    cloud_packetReceiveCallBackTable[0].recvCallBack = pf_mqtt_client->MQTT_CLIENT_receive;
    BSD_SetRecvHandlerTable(cloud_packetReceiveCallBackTable);

    //When the input comes through cli/.cfg
    if ((strcmp(ssid, "") != 0) && (strcmp(authType, "") != 0))
    {
        wifi_creds = NEW_CREDENTIALS;
        debug_printInfo(" WIFI: Connecting to AP with new credentials : %s", ssid);
    }
    //This works provided the board had connected to the AP successfully
    else
    {
        wifi_creds = DEFAULT_CREDENTIALS;
        debug_printInfo(" WIFI: Connecting to AP with the last used credentials");
    }

    // Blink Blue LED to indicate WiFi connection is pending
    LED_SetWiFi(LED_INDICATOR_PENDING);

    if (!wifi_connectToAp(wifi_creds))
    {
        LED_SetWiFi(LED_INDICATOR_ERROR);
        debug_printError(" WIFI: Failed to connect to AP");
        return false;
    }

    debug_printInfo("CLOUD: WiFi Connect timer start with %d ms", WIFI_CONNECT_TIMEOUT_MS);
    wifiTimeoutTaskHandle                     = SYS_TIME_CallbackRegisterMS(wifiTimeoutTaskcb, 0, WIFI_CONNECT_TIMEOUT_MS, SYS_TIME_SINGLE);
    shared_networking_params.cloudInitPending = 0;

    return true;
}

void CLOUD_sched(void)
{
    if (wifiTimeoutTaskTmrExpired == true)
    {
        wifiTimeoutTaskTmrExpired = false;
        wifiTimeoutTask();
    }

    if (mqttTimeoutTaskTmrExpired == true)
    {
        mqttTimeoutTaskTmrExpired = false;
        mqttTimeoutTask();
    }

    if (cloudResetTaskTmrExpired == true)
    {
        cloudResetTaskTmrExpired = false;
        cloudResetTask();
    }
}
