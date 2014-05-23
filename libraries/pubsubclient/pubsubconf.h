#ifndef pubsubconf_h
#define pubsubconf_h

#include "Arduino.h"


#define HOST "broker.mqttdashboard.com"
#define PORT 1883

#define GPRS_APN "gprs.base.be"
#define GPRS_LOGIN "base"
#define GPRS_PASSWORD "base"


// Tweak this on various arduinos
#define MQTT_MAX_TOPIC_SIZE 	64
#define MQTT_MAX_PAYLOAD_SIZE 	256
#define MQTT_MAX_LENGTH_BYTES 	4		// Length bytes are always 4 maximum
#define MQTT_BUFFER_ROOM		16		// buffer room not sure if this is needed (maybe for id bytes)
#define MQTT_MAX_PACKET_SIZE 	(MQTT_MAX_LENGTH_BYTES + MQTT_MAX_PAYLOAD_SIZE + MQTT_MAX_TOPIC_SIZE + MQTT_BUFFER_ROOM)

//Max retained subscriptions
#define MQTT_MAX_SUBSCRIPTIONS 10

// MQTT_KEEPALIVE : keepAlive interval in Seconds
#define MQTT_KEEPALIVE 	2*60


//Support for Software serial not working as of yet
#define CLIENT_HARDWARE_SERIAL

#ifndef CLIENT_HARDWARE_SERIAL
#include "libraries/SoftwareSerial/SoftwareSerial.h"
#endif

//DO Not Change this
#define MQTTPROTOCOLVERSION 3
#define MQTTCONNECT     1 << 4  // Client request to connect to Server
#define MQTTCONNACK     2 << 4  // Connect Acknowledgment
#define MQTTPUBLISH     3 << 4  // Publish message
#define MQTTPUBACK      4 << 4  // Publish Acknowledgment
#define MQTTPUBREC      5 << 4  // Publish Received (assured delivery part 1)
#define MQTTPUBREL      6 << 4  // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP     7 << 4  // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE   8 << 4  // Client Subscribe request
#define MQTTSUBACK      9 << 4  // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE 10 << 4 // Client Unsubscribe request
#define MQTTUNSUBACK    11 << 4 // Unsubscribe Acknowledgment
#define MQTTPINGREQ     12 << 4 // PING Request
#define MQTTPINGRESP    13 << 4 // PING Response
#define MQTTDISCONNECT  14 << 4 // Client is Disconnecting
#define MQTTReserved    15 << 4 // Reserved
#define MQTTQOS0        (0 << 1)
#define MQTTQOS1        (1 << 1)
#define MQTTQOS2        (2 << 1)


#define PROG_MEM  __attribute__((section(".progmem.mydata")))
#define PSTR2(s) (__extension__({const static char __c[] PROG_MEM = (s); &__c[0];}))



#endif


