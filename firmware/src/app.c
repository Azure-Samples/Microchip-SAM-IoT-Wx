/*******************************************************************************
* Copyright (C) 2019 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "wdrv_winc_client_api.h"
#include "iot_config/IoT_Sensor_Node_config.h"
#include "services/iot/cloud/crypto_client/cryptoauthlib_main.h"
#include "services/iot/cloud/crypto_client/crypto_client.h"
#include "services/iot/cloud/cloud_service.h"
#include "services/iot/cloud/wifi_service.h"
#include "services/iot/cloud/bsd_adapter/bsdWINC.h"
#include "credentials_storage/credentials_storage.h"
#include "debug_print.h"
#include "led.h"
#include "azutil.h"
//include "mqtt/mqtt_core/mqtt_core.h"
#include "services/iot/cloud/mqtt_packetPopulation/mqtt_packetPopulate.h"
#include "services/iot/cloud/mqtt_packetPopulation/mqtt_iothub_packetPopulate.h"
#include "services/iot/cloud/mqtt_packetPopulation/mqtt_iotprovisioning_packetPopulate.h"

#if CFG_ENABLE_CLI
#include "system/command/sys_command.h"
#endif

// *****************************************************************************
// *****************************************************************************
// Section: Local Function Prototypes
// *****************************************************************************
// *****************************************************************************
static void APP_SendToCloud(void);
static void APP_DataTask(void);
static void APP_WiFiConnectionStateChanged(uint8_t status);
static void APP_ProvisionRespCb(DRV_HANDLE handle, WDRV_WINC_SSID* targetSSID, WDRV_WINC_AUTH_CONTEXT* authCtx, bool status);
static void APP_DHCPAddressEventCb(DRV_HANDLE handle, uint32_t ipAddress);
static void APP_GetTimeNotifyCb(DRV_HANDLE handle, uint32_t timeUTC);
static void APP_ConnectNotifyCb(DRV_HANDLE handle, WDRV_WINC_CONN_STATE currentState, WDRV_WINC_CONN_ERROR errorCode);

static char *LED_Property[3] = {
    "On",
    "Off",
    "Blink"
    };

#ifdef CFG_MQTT_PROVISIONING_HOST
void iot_provisioning_completed(void);
#endif
// *****************************************************************************
// *****************************************************************************
// Section: Application Macros
// *****************************************************************************
// *****************************************************************************

#define APP_WIFI_SOFT_AP         0
#define APP_WIFI_DEFAULT         1
#define APP_DATATASK_INTERVAL     250L   // 250msec
#define APP_CLOUDTASK_INTERVAL    250L   // 250msec
#define APP_SW_DEBOUNCE_INTERVAL 1460000L

/* WIFI SSID, AUTH and PWD for AP */
#define APP_CFG_MAIN_WLAN_SSID ""
#define APP_CFG_MAIN_WLAN_AUTH M2M_WIFI_SEC_WPA_PSK
#define APP_CFG_MAIN_WLAN_PSK  ""

// #define CFG_APP_WINC_DEBUG 1   //define this to print WINC debug messages

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
#define SN_STRING "sn"

/* Place holder for ECC608A unique serial id */
ATCA_STATUS appCryptoClientSerialNumber;
char*       attDeviceID;
char        attDeviceID_buf[25] = "BAAAAADD1DBAAADD1D";
char        deviceIpAddress[16] = {0};

shared_networking_params_t shared_networking_params;

/* Various NTP Host servers that application relies upon for time sync */
#define WORLDWIDE_NTP_POOL_HOSTNAME "*.pool.ntp.org"
#define ASIA_NTP_POOL_HOSTNAME      "asia.pool.ntp.org"
#define EUROPE_NTP_POOL_HOSTNAME    "europe.pool.ntp.org"
#define NAMERICA_NTP_POOL_HOSTNAME  "north-america.pool.ntp.org"
#define OCEANIA_NTP_POOL_HOSTNAME   "oceania.pool.ntp.org"
#define SAMERICA_NTP_POOL_HOSTNAME  "south-america.pool.ntp.org"
#define NTP_HOSTNAME                "pool.ntp.org"

/* Driver handle for WINC1510 */
static DRV_HANDLE wdrvHandle;
static uint8_t    wifi_mode = WIFI_DEFAULT;

static SYS_TIME_HANDLE App_DataTaskHandle      = SYS_TIME_HANDLE_INVALID;
volatile bool          App_DataTaskTmrExpired  = false;
static SYS_TIME_HANDLE App_CloudTaskHandle     = SYS_TIME_HANDLE_INVALID;
volatile bool          App_CloudTaskTmrExpired = false;

static time_t     previousTransmissionTime;
volatile uint32_t telemetryInterval = CFG_DEFAULT_TELEMETRY_INTERVAL_SEC;

volatile bool iothubConnected = false;

extern pf_MQTT_CLIENT    pf_mqtt_iotprovisioning_client;
extern pf_MQTT_CLIENT    pf_mqtt_iothub_client;
extern void              sys_cmd_init();
extern userdata_status_t userdata_status;

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
extern az_iot_pnp_client pnp_client;
#else
extern az_iot_hub_client iothub_client;
#endif

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

// * PnP Values *
// The model id is the JSON document (also called the Digital Twins Model Identifier or DTMI)
// which defines the capability of your device. The functionality of the device should match what
// is described in the corresponding DTMI. Should you choose to program your own PnP capable device,
// the functionality would need to match the DTMI and you would need to update the below 'model_id'.
// Please see the sample README for more information on this DTMI.
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
const az_span device_model_id_span = AZ_SPAN_LITERAL_FROM_STR(IOT_PLUG_AND_PLAY_MODEL_ID);
#endif

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************
void APP_CloudTaskcb(uintptr_t context)
{
    App_CloudTaskTmrExpired = true;
}

void APP_DataTaskcb(uintptr_t context)
{
    App_DataTaskTmrExpired = true;
}
// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************

// React to the WIFI state change here. Status of 1 means connected, Status of 0 means disconnected
static void APP_WiFiConnectionStateChanged(uint8_t status)
{
    //debug_printInfo("  APP: WiFi Connection Status Change to %d", status);
    // If we have no AP access we want to retry
    if (status != 1)
    {
        // Restart the WIFI module if we get disconnected from the WiFi Access Point (AP)
        CLOUD_reset();
    }
}
// *****************************************************************************
// *****************************************************************************
// Section: Button interrupt handlers
// *****************************************************************************
// *****************************************************************************
void APP_SW0_Handler(void)
{
    LED_ToggleRed();
    button_press_data.sw0_press_count++;
    button_press_data.flag.sw0 = 1;
}

void APP_SW1_Handler(void)
{
    LED_ToggleRed();
    button_press_data.sw1_press_count++;
    button_press_data.flag.sw1 = 1;
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */
void APP_Initialize(void)
{
    debug_printInfo("  APP: %s()", __FUNCTION__);
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_CRYPTO_INIT;
    //uint8_t wifi_mode = WIFI_DEFAULT;
    uint32_t sw0CurrentVal = 0;
    uint32_t sw1CurrentVal = 0;
    uint32_t i             = 0;

    previousTransmissionTime = 0;

    debug_init(attDeviceID);
    LED_init();
    LED_test();
    // Blocking debounce
    for (i = 0; i < APP_SW_DEBOUNCE_INTERVAL; i++)
    {
        sw0CurrentVal += SW0_GPIO_PA00_Get();
        sw1CurrentVal += SW1_GPIO_PA01_Get();
    }

    if (sw0CurrentVal < (APP_SW_DEBOUNCE_INTERVAL / 2))
    {
        if (sw1CurrentVal < (APP_SW_DEBOUNCE_INTERVAL / 2))
        {
            strcpy(ssid, APP_CFG_MAIN_WLAN_SSID);
            strcpy(pass, APP_CFG_MAIN_WLAN_PSK);
            sprintf((char*)authType, "%d", APP_CFG_MAIN_WLAN_AUTH);
        }
        else
        {
            wifi_mode = WIFI_SOFT_AP;
        }
    }
    /* Open I2C driver client */
    ADC_Enable();
    LED_test();
    sys_cmd_init();   // CLI init

#if (CFG_APP_WINC_DEBUG == 1)
    WDRV_WINC_DebugRegisterCallback(debug_printer);
#endif
    EIC_CallbackRegister(EIC_PIN_0, (EIC_CALLBACK)APP_SW0_Handler, 0);
    EIC_InterruptEnable(EIC_PIN_0);
    EIC_CallbackRegister(EIC_PIN_1, (EIC_CALLBACK)APP_SW1_Handler, 0);
    EIC_InterruptEnable(EIC_PIN_1);

    userdata_status.as_uint8 = 0;
}

static void APP_ConnectNotifyCb(DRV_HANDLE handle, WDRV_WINC_CONN_STATE currentState, WDRV_WINC_CONN_ERROR errorCode)
{
    debug_printTrace("  APP: APP_ConnectNotifyCb %d", currentState);

    if (WDRV_WINC_CONN_STATE_CONNECTED == currentState)
    {
        WiFi_ConStateCb(M2M_WIFI_CONNECTED);
    }
    else if (WDRV_WINC_CONN_STATE_DISCONNECTED == currentState)
    {
        WiFi_ConStateCb(M2M_WIFI_DISCONNECTED);
    }
}

static void APP_GetTimeNotifyCb(DRV_HANDLE handle, uint32_t timeUTC)
{
    //checking > 0 is not recommended, even if getsystime returns null, utctime value will be > 0
    if (timeUTC != 0x86615400U)
    {
        tstrSystemTime pSysTime;
        struct tm      theTime;

        WDRV_WINC_UTCToLocalTime(timeUTC, &pSysTime);
        theTime.tm_hour  = pSysTime.u8Hour;
        theTime.tm_min   = pSysTime.u8Minute;
        theTime.tm_sec   = pSysTime.u8Second;
        theTime.tm_year  = pSysTime.u16Year - 1900;
        theTime.tm_mon   = pSysTime.u8Month - 1;
        theTime.tm_mday  = pSysTime.u8Day;
        theTime.tm_isdst = 0;
        RTC_RTCCTimeSet(&theTime);
    }
}

static void APP_DHCPAddressEventCb(DRV_HANDLE handle, uint32_t ipAddress)
{
    LED_SetWiFi(LED_INDICATOR_SUCCESS);

    memset(deviceIpAddress, 0, sizeof(deviceIpAddress));

    sprintf(deviceIpAddress, "%lu.%lu.%lu.%lu", 
                        (0x0FF & (ipAddress)),
                        (0x0FF & (ipAddress >> 8)),
                        (0x0FF & (ipAddress >> 16)),
                        (0x0FF & (ipAddress >> 24)));

    debug_printGood("  APP: DHCP IP Address %s", deviceIpAddress);

    shared_networking_params.haveIpAddress = 1;
    shared_networking_params.haveERROR     = 0;
    shared_networking_params.reported      = 0;
}

static void APP_ProvisionRespCb(DRV_HANDLE handle, WDRV_WINC_SSID* targetSSID,
                                WDRV_WINC_AUTH_CONTEXT* authCtx, bool status)
{
    uint8_t  sectype;
    uint8_t* ssid;
    uint8_t* password;

    debug_printInfo("  APP: %s()", __FUNCTION__);

    if (status == M2M_SUCCESS)
    {
        sectype  = authCtx->authType;
        ssid     = targetSSID->name;
        password = authCtx->authInfo.WPAPerPSK.key;
        WiFi_ProvisionCb(sectype, ssid, password);
    }
}

void APP_Tasks(void)
{
    switch (appData.state)
    {
        case APP_STATE_CRYPTO_INIT: {

            char serialNumber_buf[25];

            shared_networking_params.allBits = 0;
            debug_setPrefix(attDeviceID);
            cryptoauthlib_init();

            if (cryptoDeviceInitialized == false)
            {
                debug_printError("  APP: CryptoAuthInit failed");
            }

#ifdef HUB_DEVICE_ID
            attDeviceID = HUB_DEVICE_ID;
#else
            appCryptoClientSerialNumber = CRYPTO_CLIENT_printSerialNumber(serialNumber_buf);
            if (appCryptoClientSerialNumber != ATCA_SUCCESS)
            {
                switch (appCryptoClientSerialNumber)
                {
                    case ATCA_GEN_FAIL:
                        debug_printError("  APP: DeviceID generation failed, unspecified error");
                        break;
                    case ATCA_BAD_PARAM:
                        debug_printError("  APP: DeviceID generation failed, bad argument");
                    default:
                        debug_printError("  APP: DeviceID generation failed");
                        break;
                }
            }
            else
            {
                // To use Azure provisioning service, attDeviceID should match with the device cert CN,
                // which is the serial number of ECC608 prefixed with "sn" if you are using the
                // the microchip provisioning tool for PIC24.
                strcpy(attDeviceID_buf, SN_STRING);
                strcat(attDeviceID_buf, serialNumber_buf);
                attDeviceID = attDeviceID_buf;
            }
#endif
#if CFG_ENABLE_CLI
            set_deviceId(attDeviceID);
#endif
            debug_setPrefix(attDeviceID);
            CLOUD_setdeviceId(attDeviceID);
            appData.state = APP_STATE_WDRV_INIT;
            break;
        }

        case APP_STATE_WDRV_INIT: {
            if (SYS_STATUS_READY == WDRV_WINC_Status(sysObj.drvWifiWinc))
            {
                appData.state = APP_STATE_WDRV_INIT_READY;
            }
            break;
        }

        case APP_STATE_WDRV_INIT_READY: {
            wdrvHandle = WDRV_WINC_Open(0, 0);

            if (DRV_HANDLE_INVALID != wdrvHandle)
            {
                appData.state = APP_STATE_WDRV_OPEN;
            }

            break;
        }

        case APP_STATE_WDRV_OPEN: {
            m2m_wifi_configure_sntp((uint8_t*)NTP_HOSTNAME, strlen(NTP_HOSTNAME), SNTP_ENABLE_DHCP);
            m2m_wifi_enable_sntp(1);
            WDRV_WINC_DCPT* pDcpt      = (WDRV_WINC_DCPT*)wdrvHandle;
            pDcpt->pfProvConnectInfoCB = APP_ProvisionRespCb;

            debug_printInfo("  APP: WiFi Mode %d", wifi_mode);
            wifi_init(APP_WiFiConnectionStateChanged, wifi_mode);

#ifdef CFG_MQTT_PROVISIONING_HOST
            pf_mqtt_iotprovisioning_client.MQTT_CLIENT_task_completed = iot_provisioning_completed;
            CLOUD_init_host(CFG_MQTT_PROVISIONING_HOST, attDeviceID, &pf_mqtt_iotprovisioning_client);
#else
            CLOUD_init_host(hub_hostname, attDeviceID, &pf_mqtt_iothub_client);
#endif   //CFG_MQTT_PROVISIONING_HOST
            if (wifi_mode == WIFI_DEFAULT)
            {
                /* Enable use of DHCP for network configuration, DHCP is the default
                but this also registers the callback for notifications. */
                WDRV_WINC_IPUseDHCPSet(wdrvHandle, &APP_DHCPAddressEventCb);

                debug_printGood("  APP: registering APP_CloudTaskcb");
                App_CloudTaskHandle = SYS_TIME_CallbackRegisterMS(APP_CloudTaskcb, 0, APP_CLOUDTASK_INTERVAL, SYS_TIME_PERIODIC);
                WDRV_WINC_BSSReconnect(wdrvHandle, &APP_ConnectNotifyCb);
                WDRV_WINC_SystemTimeGetCurrent(wdrvHandle, &APP_GetTimeNotifyCb);
            }

            appData.state = APP_STATE_WDRV_ACTIV;
            break;
        }

        case APP_STATE_WDRV_ACTIV: {
            if (App_CloudTaskTmrExpired == true)
            {
                App_CloudTaskTmrExpired = false;
                CLOUD_task();
            }

            if (App_DataTaskTmrExpired == true)
            {
                App_DataTaskTmrExpired = false;
                APP_DataTask();
            }

            CLOUD_sched();
            wifi_sched();
            MQTT_sched();
            break;
        }
        default: {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}


//static int skipper = 0;

// This gets called by the scheduler approximately every 100ms
static void APP_DataTask(void)
{

    // Example of how to send data when MQTT is connected every 1 second based on the system clock
    if (CLOUD_isConnected())
    {
        //if (skipper++ % 3 == 0)
        {
            // send telemetry
            APP_SendToCloud();
        }

        check_button_status();

        if (shared_networking_params.reported == 0)
        {
            twin_properties_t twin_properties;

            init_twin_data(&twin_properties);

            strcpy(twin_properties.ip_address, deviceIpAddress);
            twin_properties.flag.ip_address_updated = 1;

            if (az_result_succeeded(send_reported_property(&twin_properties)))
            {
                shared_networking_params.reported = 1;
            }
            else
            {
                debug_printError("  APP: Failed to report IP Address property");
            }
        }

        if (userdata_status.as_uint8 != 0)
        {
            uint8_t i;
            // received user data via UART
            // update reported property
            for (i = 0; i < 8; i++)
            {
                uint8_t tmp = userdata_status.as_uint8;
                if (tmp & (1 << i))
                {
                    debug_printInfo("  APP: User Data Slot %d changed", i + 1);
                }
            }
            userdata_status.as_uint8 = 0;
        }
    }
    else
    {
        debug_printWarn("  APP: Not Connected");
    }

    if (shared_networking_params.haveAPConnection)
    {
        LED_SetBlue(LED_STATE_HOLD);
    }
    else
    {
        LED_SetBlue(LED_STATE_OFF);
    }

    if (shared_networking_params.haveERROR)
    {
        LED_SetWiFi(LED_INDICATOR_ERROR);
    }
    else
    {
        LED_SetWiFi(LED_INDICATOR_SUCCESS);
    }

    if (CLOUD_isConnected())
    {
        LED_SetCloud(LED_INDICATOR_SUCCESS);
    }
    else
    {
        LED_SetCloud(LED_INDICATOR_ERROR);
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Functions to interact with Azure IoT Hub and DPS
// *****************************************************************************
// *****************************************************************************

/**********************************************
* Command (Direct Method)
**********************************************/
void APP_ReceivedFromCloud_methods(uint8_t* topic, uint8_t* payload)
{
    az_result                         rc;
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    az_iot_pnp_client_command_request command_request;
#else
    az_iot_hub_client_method_request method_request;
#endif

    debug_printInfo("  APP: %s() Topic %s Payload %s", __FUNCTION__, topic, payload);

    if (topic == NULL)
    {
        debug_printError("  APP: Command topic empty");
        return;
    }

    az_span command_topic_span = az_span_create(topic, strlen((char*)topic));

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
    rc = az_iot_pnp_client_commands_parse_received_topic(&pnp_client, command_topic_span, &command_request);
#else
    rc = az_iot_hub_client_methods_parse_received_topic(&iothub_client, command_topic_span, &method_request);
#endif

    if (az_result_succeeded(rc))
    {
        debug_printTrace("  APP: Command Topic  : %s", az_span_ptr(command_topic_span));
#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
        debug_printTrace("  APP: Command Name   : %s", az_span_ptr(command_request.command_name));
#else
        debug_printTrace("  APP: Method Name   : %s", az_span_ptr(method_request.name));
#endif
        debug_printTrace("  APP: Command Payload: %s", (char*)payload);

#ifdef IOT_PLUG_AND_PLAY_MODEL_ID
        process_direct_method_command(payload, &command_request);
#else
        process_direct_method_command(payload, &method_request);
#endif

    }
    else
    {
        debug_printError("  APP: Command from unknown topic: '%s' return code 0x%08x.", az_span_ptr(command_topic_span), rc);
    }
}

/**********************************************
* Properties (Device Twin)
**********************************************/
void APP_ReceivedFromCloud_patch(uint8_t* topic, uint8_t* payload)
{
    az_result         rc;
    twin_properties_t twin_properties;

    init_twin_data(&twin_properties);

    twin_properties.flag.is_initial_get = 0;

    debug_printInfo("  APP: %s() Payload %s", __FUNCTION__, payload);

    if (az_result_failed(rc = process_device_twin_property(topic, payload, &twin_properties)))
    {
        // If the item can't be found, the desired temp might not be set so take no action
        debug_printError("  APP: Could not parse desired property, return code 0x%08x\n", rc);
    }
    else
    {
        if (twin_properties.flag.yellow_led_found == 1)
        {
            debug_printInfo("  APP: Found led_y Value '%s' (%d)",
                            LED_Property[twin_properties.desired_led_yellow - 1],
                            twin_properties.desired_led_yellow);
        }

        if (twin_properties.flag.telemetry_interval_found == 1)
        {
            debug_printInfo("  APP: Found telemetryInterval value '%d'", telemetryInterval);
        }

        update_leds(&twin_properties);
        rc = send_reported_property(&twin_properties);

        if (az_result_failed(rc))
        {
            debug_printError("  APP: send_reported_property() failed 0x%08x", rc);
        }
    }
}


void APP_ReceivedFromCloud_twin(uint8_t* topic, uint8_t* payload)
{
    az_result         rc;
    twin_properties_t twin_properties;

    init_twin_data(&twin_properties);

    if (topic == NULL)
    {
        debug_printWarn("  APP: Twin topic empty");
        return;
    }

    debug_printTrace("  APP: %s() Payload %s", __FUNCTION__, payload);

    if (az_result_failed(rc = process_device_twin_property(topic, payload, &twin_properties)))
    {
        // If the item can't be found, the desired temp might not be set so take no action
        debug_printError("  APP: Could not parse desired property, return code 0x%08x\n", rc);
    }
    else
    {
        if (twin_properties.flag.is_initial_get)
        {
            iothubConnected = true;
        }

        if (twin_properties.flag.yellow_led_found == 1)
        {
            debug_printInfo("  APP: Found led_y Value '%s' (%d)",
                            LED_Property[twin_properties.desired_led_yellow - 1],
                            twin_properties.desired_led_yellow);
        }

        if (twin_properties.flag.telemetry_interval_found == 1)
        {
            debug_printInfo("  APP: Found telemetryInterval Value '%d'", telemetryInterval);
        }

        update_leds(&twin_properties);
        rc = send_reported_property(&twin_properties);

        if (az_result_failed(rc))
        {
            debug_printError("  APP: send_reported_property() failed 0x%08x", rc);
        }
    }

    //debug_printInfo("  APP: << %s() rc = 0x%08x", __FUNCTION__, rc);
}

/**********************************************
* Read Temperature Sensor value
**********************************************/
float APP_GetTempSensorValue(void)
{
    float retVal = 0;
    /* TA: AMBIENT TEMPERATURE REGISTER ADDRESS: 0x5 */
    uint8_t registerAddr = 0x5;
    /* Temp sensor read buffer */
    uint8_t rxBuffer[2];

    while (SERCOM3_I2C_IsBusy() == true)
    {
        /* Wait for the I2C */
    }

    if (SERCOM3_I2C_WriteRead(0x18, (uint8_t*)&registerAddr, 1, (uint8_t*)rxBuffer, 2) == true)
    {
        while (SERCOM3_I2C_IsBusy() == true)
        {
            /* Wait for the I2C transfer to complete */
        }

        /* Transfer complete. Check if the transfer was successful */
        if (SERCOM3_I2C_ErrorGet() == SERCOM_I2C_ERROR_NONE)
        {
            rxBuffer[0] = rxBuffer[0] & 0x1F;   //Clear flag bits
            if ((rxBuffer[0] & 0x10) == 0x10)
            {
                rxBuffer[0] = rxBuffer[0] & 0x0F;   //Clear SIGN
                retVal      = 256.0 - (rxBuffer[0] * 16.0 + rxBuffer[1] / 16.0);
            }
            else
            {
                retVal = ((rxBuffer[0] * 16.0) + (rxBuffer[1] / 16.0));
            }
        }
        else
        {
            LED_SetRed(LED_STATE_BLINK_SLOW);
        }
    }
    return retVal;
}

/**********************************************
* Build light sensor value
**********************************************/
int32_t APP_GetLightSensorValue(void)
{
    uint32_t input_voltage = 0;
    int32_t  retVal        = 0;
    uint16_t adc_count     = 0;

    while (!ADC_ConversionStatusGet())
    {
        //Obtain result from Light sensor
    }

    /* Read the ADC result */
    adc_count     = ADC_ConversionResultGet();
    input_voltage = adc_count * 1650 / 4095U;
    retVal        = input_voltage;

    return retVal;
}

/**********************************************
* Entry point for telemetry
**********************************************/
void APP_SendToCloud(void)
{
    if (iothubConnected)
    {
        send_telemetry_message();
    }
}

/**********************************************
* Callback functions for MQTT
**********************************************/
void iot_connection_completed(void)
{
    debug_printGood("  APP: %s()", __FUNCTION__);

    LED_SetCloud(LED_INDICATOR_SUCCESS);

    App_DataTaskHandle = SYS_TIME_CallbackRegisterMS(APP_DataTaskcb, 0, APP_DATATASK_INTERVAL, SYS_TIME_PERIODIC);
}

#ifdef CFG_MQTT_PROVISIONING_HOST
void iot_provisioning_completed(void)
{
    debug_printGood("  APP: %s()", __FUNCTION__);
    pf_mqtt_iothub_client.MQTT_CLIENT_task_completed = iot_connection_completed;
    CLOUD_init_host(hub_hostname, attDeviceID, &pf_mqtt_iothub_client);
    CLOUD_reset();
    LED_SetCloud(LED_INDICATOR_PENDING);
}
#endif   //CFG_MQTT_PROVISIONING_HOST

/*******************************************************************************
 End of File
 */
