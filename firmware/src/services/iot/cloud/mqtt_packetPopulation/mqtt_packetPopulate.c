/*
    \file   mqtt_packetParameters.c

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

char* hub_device_key = HUB_DEVICE_KEY;
char* hub_hostname = CFG_MQTT_HUB_HOST;
char mqtt_password_buf[512];
char mqtt_username_buf[200];

char digit_to_hex(char number)
{
	return (char)(number + (number < 10 ? '0' : 'A' - 10));
}

char* url_encode_rfc3986(char* s, char* dest, size_t dest_len) {

	for (; *s && dest_len > 1; s++) {

		if (isalnum(*s) || *s == '~' || *s == '-' || *s == '.' || *s == '_')
		{
			*dest++ = *s;
		}
		else if (dest_len < 4)
		{
			break;
		}
		else
		{
			*dest++ = '%';
			*dest++ = digit_to_hex(*s / 16);
			*dest++ = digit_to_hex(*s % 16);
		}
	}

	*dest++ = '\0';
	return dest;
}
