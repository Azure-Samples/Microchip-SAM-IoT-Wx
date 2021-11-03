/*
 * cloud_service.h
 *
 * Created: 9/27/2018 2:25:12 PM
 *  Author: C14674
 */

#ifndef CLOUD_SERVICE_H_
#define CLOUD_SERVICE_H_

#include <stdbool.h>
#include "mqtt_packetPopulation/mqtt_packetPopulate.h"

// this must be = to MAX_SUPPORTED_SOCKETS
#define CLOUD_PACKET_RECV_TABLE_SIZE 2

void CLOUD_init_host(char* host, char* deviceId, pf_MQTT_CLIENT* pf_table);
void CLOUD_reset(void);
void CLOUD_subscribe(void);
void CLOUD_disconnect(void);
bool CLOUD_isConnected(void);
void CLOUD_publishData(uint8_t* topic, uint8_t* payload, uint16_t payload_len, int qos);
void CLOUD_task(void);
void CLOUD_sched(void);
void dnsHandler(uint8_t* domainName, uint32_t serverIP);
void CLOUD_setdeviceId(char* id);
uint8_t reInit(void);

#endif /* CLOUD_SERVICE_H_ */
