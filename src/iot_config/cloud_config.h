#ifndef CLOUD_CONFIG_H
#define CLOUD_CONFIG_H

// <h> Cloud Configuration

// <s> project id
// <i> Azure IoT Hub
// <id> project_id
#define CFG_PROJECT_ID "sam-iot"

// <s> mqtt port
// <i> mqtt port value
// <id> mqtt_port
#define CFG_MQTT_PORT 8883

// <s> mqtt hub host
// <i> mqtt hub host address
// <id> mqtt_hub_host
#define CFG_MQTT_HUB_HOST "myiothub.azure-devices.net"

// <s> mqtt provisioning host
// <i> mqtt provisioning host address
// <id> provisioning host
#define CFG_MQTT_PROVISIONING_HOST "global.azure-devices-provisioning.net"

// <s> provisioning id scope
// <i> provisioning id scope of the DPS service
// <id> id
#define PROVISIONING_ID_SCOPE "0nexxxxxxx"

// </h>


// <h> WLAN Configuration

// <s> SSID
// <i> Target WLAN SSID
// <id> main_wlan_ssid
#define CFG_MAIN_WLAN_SSID "GUESTNETWORK"

// <y> Authentication
// <i> Target WLAN Authentication
// <M2M_WIFI_SEC_INVALID"> Invalid security type
// <M2M_WIFI_SEC_OPEN"> Wi-Fi network is not secured
// <M2M_WIFI_SEC_WPA_PSK"> Wi-Fi network is secured with WPA/WPA2 personal(PSK)
// <M2M_WIFI_SEC_WEP"> Security type WEP (40 or 104) OPEN OR SHARED
// <M2M_WIFI_SEC_802_1X"> Wi-Fi network is secured with WPA/WPA2 Enterprise.IEEE802.1x user-name/password authentication
// <id> main_wlan_auth
#define CFG_MAIN_WLAN_AUTH M2M_WIFI_SEC_WPA_PSK

// <s> Password
// <i> Target WLAN password
// <id> main_wlan_psk
#define CFG_MAIN_WLAN_PSK "password"

// </h>



#endif // CLOUD_CONFIG_H
