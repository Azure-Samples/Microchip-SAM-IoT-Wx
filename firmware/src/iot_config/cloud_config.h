#ifndef CLOUD_CONFIG_H
#define CLOUD_CONFIG_H

// <h> Cloud Configuration

// <s> mqtt port
// <i> mqtt port value
// <id> mqtt_port
#define CFG_MQTT_PORT AZ_IOT_DEFAULT_MQTT_CONNECT_PORT

// <s> mqtt hub host
// <i> mqtt hub host address
// <id> mqtt_hub_host
#define CFG_MQTT_HUB_HOST "<INSERT HUB>"

// <s> mqtt provisioning host
// <i> mqtt provisioning host address
// <id> provisioning host
#define CFG_MQTT_PROVISIONING_HOST "global.azure-devices-provisioning.net"

// <s> provisioning id scope
// <i> provisioning id scope of the DPS service
// <id> id
#define PROVISIONING_ID_SCOPE ""

// </h>

#endif   // CLOUD_CONFIG_H
