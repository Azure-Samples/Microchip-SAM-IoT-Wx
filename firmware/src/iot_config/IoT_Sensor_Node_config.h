#ifndef IOT_SENSOR_NODE_CONFIG_H
#define IOT_SENSOR_NODE_CONFIG_H

#define CFG_DEFAULT_TELEMETRY_INTERVAL_SEC 10

#define IOT_DEBUG_PRINT 1

//#define CFG_MQTT_DEBUG_MSG 1    //set to enable debug print messages MQTT

#define CFG_ENABLE_CLI 1

#define CFG_APP_WINC_DEBUG 1

#define CFG_LED_DEBUG 0

// Comment out or remove IOT_PLUG_AND_PLAY_MODEL_ID to run as non-IoT Plug and Play client
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:Microchip:SAM_IoT_WM;1"

#endif   // IOT_SENSOR_NODE_CONFIG_H
