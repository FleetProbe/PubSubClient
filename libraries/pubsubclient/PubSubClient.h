/*
 * PubSubClient.h
 *
 *  Created on: May 15, 2014
 *      Author: gerdmestdagh
 */

#ifndef PUBSUBCLIENT_H_
#define PUBSUBCLIENT_H_
#include "client/sim900client/Sim900Client.h"
#include "ClientUtilities.h"
#include "pubsubconf.h"



class PubSubClient {
private:
	const char *domain;
	int port;
	Sim900Client *_client;
	unsigned char header;
	uint8_t length_bytes[4];
	uint8_t buffer[MQTT_MAX_PACKET_SIZE];
	int subscriptioncount;
	char *subscriptions[MQTT_MAX_SUBSCRIPTIONS];
	char topic[MQTT_MAX_TOPIC_SIZE];
	char payload[MQTT_MAX_PAYLOAD_SIZE];
	char id[2];
	char sentid[2];
	unsigned char expected_header;
	bool isConnected;

	int writeString(char *string, int length);
	int calculateLengthBytes(int length);

	int incrementSentId() {
		int value = (sentid[0] << 8) + sentid[1];
		value++;
		sentid[0] = value >> 8;
		sentid[1] = value & 0xFF;
		return value;
	}

	int append(uint8_t *destination, uint8_t *source, int pos, int length) {
		for (int i = 0; i < length; i++) {
			destination[pos++] = source[i];
		}
		return pos;
	}

	int write(int length, int length_bytes_length) {
		uint8_t buf[MQTT_MAX_PACKET_SIZE];
		memset(buf, '\0', MQTT_MAX_PACKET_SIZE);
		buf[0] = header;
		if ((header & 0x06) == MQTTQOS0) {
			expected_header = 0;
		} else {
			expected_header = (header & 0xF0) + 0x10;
		}
		int pos = 1;
		pos = append(buf, length_bytes, pos, length_bytes_length);
		pos = append(buf, buffer, pos, length);
		_client->write(buf, pos);
		debugPrint(buf, pos);
		return pos;
	}

	int read() {
		header = 0;
		memset(length_bytes, '\0', 4);
		memset(buffer, '\0', MQTT_MAX_PACKET_SIZE);
		int len = _client->read(buffer, MQTT_MAX_PACKET_SIZE);
		if(strstr((char*)buffer,"CLOSED") != NULL){
			isConnected  = false;
		}
		debugPrint(buffer, len);
		return len;
	}

	int parseVariableLength(uint8_t *counter, uint8_t* buf) {
		int result = 0;
		int multiplier = 1;
		do {
			int c = buf[(*counter)++];
			result += (c & 0x7F) * multiplier;
			multiplier *= 128;
			if (c >> 7 == 0) {
				return result;
			}
		} while (*counter < 256);
		return -1;
	}

	void debugPrint(uint8_t *buffer, int length) {
		if (length > 0) {
			for (int i = 0; i < length; i++) {
				char c = buffer[i];
				if (c > ' ' && i != 0) {
					Serial.print(c);
				} else {
					char hex[7];
					sprintf(hex, "[%02X]", c);
					Serial.print(hex);
				}
			}
			Serial.print('\n');
		}
	}


public:
	PubSubClient(const char *domain, int port);
	bool connect(char *id, char *user, char *pass, char* willTopic,
			uint8_t willQos, uint8_t willRetain, char* willMessage);
	bool publish(char* topic, char* payload, boolean retained, int qos);
	bool subscribe(char* topic, uint8_t qos);
	bool run();
	virtual ~PubSubClient();
	bool connected(){
		if(_client != NULL)
			return isConnected && _client->connected();
		else
			return false;
	}
};

#endif /* PUBSUBCLIENT_H_ */
