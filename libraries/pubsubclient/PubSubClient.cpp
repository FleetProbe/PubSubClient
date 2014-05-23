/*
 * PubSubClient.cpp
 *
 *  Created on: May 15, 2014
 *      Author: gerdmestdagh
 */

#include "PubSubClient.h"

PubSubClient::PubSubClient(const char *domain, int port) {
	this->domain = domain;
	this->port = port;
#ifndef CLIENT_HARDWARE_SERIAL
	_client = new Sim900Client(new SoftwareSerial(6,4));
#else
	_client = new Sim900Client(&Serial2);
#endif
	subscriptioncount = 0;
	sentid[0] = 0x00;
	sentid[1] = 0x01;
}
bool PubSubClient::connect(char *id, char *user, char *pass, char* willTopic, uint8_t willQos, uint8_t willRetain, char* willMessage) {
	printPSTR(PSTR2("Connecting"));
	int result = 0;

	if (domain[0] != '\0') {
		while (!_client->connect(this->domain, this->port))
			;
	} else {
		printPSTR(PSTR2("error domain is null"));
		return 0;
	}

	header = MQTTCONNECT;
	int length = 0;
	memset(buffer, '\0', MQTT_MAX_PACKET_SIZE);
	char connectString[] = { 0x00, 0x06, 'M', 'Q', 'I', 's', 'd', 'p', MQTTPROTOCOLVERSION };
	for (int j = 0; j < 9; j++) {
		buffer[length++] = connectString[j];
	}
	char c;
	if (willTopic) {
		c = 0x06 | (willQos << 3) | (willRetain << 5);
	} else {
		c = 0x02;
	}
	if (user != NULL) {
		c = c | 0x80;
		if (pass != NULL) {
			c = c | (0x80 >> 1);
		}
	}
	buffer[length++] = c;
	buffer[length++] = ((MQTT_KEEPALIVE) >> 8);
	buffer[length++] = ((MQTT_KEEPALIVE) & 0xFF);
	length = writeString(id, length);
	if (user != NULL) {
		length = writeString(user, length);
		if (pass != NULL) {
			length = writeString(pass, length);
		}
	}
	int length_bytes_length = calculateLengthBytes(length);
	unsigned long start_connection_time = millis();
	write(length, length_bytes_length);
	while (!(_client->available() > 0) && millis() - start_connection_time < 5000)
		;
	subscriptioncount = 0;
	if (run()) {
		return true;
	}
	return false;
}

bool PubSubClient::publish(char* topic, char* payload, boolean retained, int qos) {

	memset(buffer, '\0', MQTT_MAX_PACKET_SIZE);
	int length = 0;
	length = writeString(topic, length);

	if (qos > 0) {
		buffer[length++] = sentid[0];
		buffer[length++] = sentid[1];
	}
	uint16_t i;
	for (i = 0; i < strlen((const char*) payload); i++) {
		buffer[length++] = payload[i];
	}
	header = 0;
	header |= MQTTPUBLISH;
	header |= qos;
	if (retained) {
		header |= 1;
	}

	int length_bytes_length = calculateLengthBytes(length);
	write(length, length_bytes_length);
	if (run()) {
		return true;
	}
	return false;
}

bool PubSubClient::run() {
	int skip_process = 0;
	memset(buffer, '\0', MQTT_MAX_PACKET_SIZE);
	if (expected_header != 0) {
		unsigned long connection_time_out_start = millis();
		bool timed_out = false;
		while (expected_header != (buffer[0] & 0xF0) && !timed_out) {
			skip_process = read();
			if (millis() - connection_time_out_start < 3000) {
				timed_out = true;
				expected_header = 0;
			}
		}
	} else {
		if (_client->available() > 0)
			skip_process = read();
	}
	header = buffer[0];

	switch (header & 0xF0) {
	case MQTTCONNECT: {
		printPSTR(PSTR2("Connect"));
		break;
	}
	case MQTTCONNACK: {
		printPSTR(PSTR2("ConnAck"));
		if (buffer[1] == 0x02) {
			switch (buffer[3]) {
			case 0:
				printPSTR(PSTR2("Connection Accepted"));
				isConnected = true;
				return true;
				break;
			case 1:
				printPSTR(PSTR2("Connection Refused: unacceptable protocol version"));
				break;
			case 2:
				printPSTR(PSTR2("Connection Refused: identifier rejected"));
				break;
			case 3:
				printPSTR(PSTR2("Connection Refused: server unavailable"));
				break;
			case 4:
				printPSTR(PSTR2("Connection Refused: bad user name or password"));
				break;
			case 5:
				printPSTR(PSTR2("Connection Refused: not authorized"));
				break;
			default:
				printPSTR(PSTR2("Connection Refused: undefined"));
				break;
			}
			isConnected = false;
			return false;
		}
		break;
		expected_header = 0;
		break;
	}
	case MQTTPUBLISH: {
		printPSTR(PSTR2("Publish"));
		int pos = 1;
		uint8_t length_bytes_length = 1;
		int msg_length = parseVariableLength(&length_bytes_length, buffer);
		length_bytes_length -= 1;
		pos += length_bytes_length;
		int topic_length = (buffer[pos] << 8) + buffer[pos + 1];

		pos += 2;
		memset(topic, '\0', MQTT_MAX_TOPIC_SIZE);
		memset(id, '\0', 2);
		memset(payload, '\0', MQTT_MAX_PAYLOAD_SIZE);
		int x = 0;
		for (int i = pos; i < topic_length + pos; i++) {
			if (x < MQTT_MAX_TOPIC_SIZE)
				topic[x++] = buffer[i];
		}
		Serial.println(topic);
		pos += topic_length;
		switch (header & 0x06) {
		case MQTTQOS1: {
			id[0] = buffer[pos];
			id[1] = buffer[pos + 1];
			pos += 2;
			break;
		}
		}
		if (skip_process != -2) {

			x = 0;
			for (int i = pos; i < msg_length + length_bytes_length + 1; i++) {
				if (x < MQTT_MAX_PAYLOAD_SIZE)
					payload[x++] = buffer[i];
			}
			pos += x;
			Serial.println(payload);

			if (pos - length_bytes_length - 1 != msg_length) {
				printPSTR(PSTR2("Message corrupt"));
			} else {
				if ((header & 0x06) == MQTTQOS1) {
					int i = 0;
					do {
						//TODO wildcards not checked
						if (subscriptions[i][strlen(subscriptions[i]) - 1] == '#') {
							printPSTR(PSTR2("Wildcard # detected"));
							subscriptions[i][strlen(subscriptions[i]) - 1] = 0;
							subscriptions[i][strlen(subscriptions[i]) - 2] = 0;
						}
						if (strstr(topic, subscriptions[i])) {
							uint8_t response_buffer[5];
							memset(response_buffer, '\0', 5);
							response_buffer[0] = MQTTPUBACK;
							response_buffer[1] = 0x02;
							response_buffer[2] = id[0];
							response_buffer[3] = id[1];
							int msgID = (response_buffer[2] << 8) + response_buffer[3];
							if (msgID > 0) {
								_client->write(response_buffer, 4);
							}
						}
						i++;
					} while (i < subscriptioncount);
				}
			}
		} else {
			if ((header & 0x06) == MQTTQOS1) {
				int i = 0;
				do {
					//TODO wildcards not checked
					if (subscriptions[i][strlen(subscriptions[i]) - 1] == '#') {
						printPSTR(PSTR2("Wildcard # detected"));
						subscriptions[i][strlen(subscriptions[i]) - 1] = 0;
						subscriptions[i][strlen(subscriptions[i]) - 2] = 0;
					}
					if (strstr(topic, subscriptions[i])) {
						uint8_t response_buffer[5];
						memset(response_buffer, '\0', 5);
						response_buffer[0] = MQTTPUBACK;
						response_buffer[1] = 0x02;
						response_buffer[2] = id[0];
						response_buffer[3] = id[1];
						int msgID = (response_buffer[2] << 8) + response_buffer[3];
						if (msgID > 0) {
							_client->write(response_buffer, 4);
						}
					}
					i++;
				} while (i < subscriptioncount);
			}
		}
		break;
	}
	case MQTTPUBACK: {
		printPSTR(PSTR2("PubAck"));
		if (buffer[1] == 0x02 && buffer[2] == sentid[0] && buffer[3] == sentid[1]) {
			printPSTR(PSTR2("PubAck Accepted"));
			return true;
		}
		expected_header = 0;
		return false;
		break;
	}
	case MQTTPUBREC: {
		printPSTR(PSTR2("PubRec"));
		break;
	}
	case MQTTPUBREL: {
		printPSTR(PSTR2("PubRel"));
		break;
	}
	case MQTTPUBCOMP: {
		printPSTR(PSTR2("PubComp"));
		break;
	}
	case MQTTSUBSCRIBE: {
		printPSTR(PSTR2("Subscribe"));
		break;
	}
	case MQTTSUBACK: {
		printPSTR(PSTR2("SubAck Received"));
		if (buffer[1] == 0x03 && buffer[2] == sentid[0] && buffer[3] == sentid[1] && buffer[4] == 0x02) {
			printPSTR(PSTR2("SubAck Accepted"));
			return true;
		}
		expected_header = 0;
		return false;
		break;
	}
	case MQTTUNSUBSCRIBE: {
		printPSTR(PSTR2("Unsubscribe"));
		break;
	}
	case MQTTUNSUBACK: {
		printPSTR(PSTR2("UnSubAck"));
		break;
	}
	case MQTTPINGREQ: {
		break;
	}
	case MQTTDISCONNECT: {
		printPSTR(PSTR2("Disconnect"));
		break;
	}
	default: {
		break;
	}
	}

}
bool PubSubClient::subscribe(char* topic, uint8_t qos) {
	int length = 0;
	memset(buffer, '\0', MQTT_MAX_PACKET_SIZE);

	header = MQTTSUBSCRIBE;
	header |= qos;
	buffer[length++] = sentid[0];
	buffer[length++] = sentid[1];
	length = writeString(topic, length);

	buffer[length++] = qos;
	int length_bytes_length = calculateLengthBytes(length);
	write(length, length_bytes_length);

	if (qos > 0) {
		subscriptions[subscriptioncount] = topic;
		subscriptioncount++;
	}
	if (run()) {
		return true;
	}
	return false;
}

int PubSubClient::calculateLengthBytes(int length) {
	memset(length_bytes, '\0', 4);
	uint8_t digit;
	uint8_t pos = 0;
	char lenBuf[4];
	int len = length;
	do {
		digit = len % 128;
		len = len / 128;
		if (len > 0) {
			digit |= 0x80;
		}
		lenBuf[pos++] = digit;
	} while (len > 0);

	for (int i = 0; i < pos; i++) {
		length_bytes[i] = lenBuf[i];
	}
	return pos;
}

int PubSubClient::writeString(char *string, int length) {
	char* idp = string;
	uint16_t i = 0;
	length += 2;
	while (*idp) {
		buffer[length++] = *idp++;
		i++;
	}
	buffer[length - i - 2] = (i >> 8);
	buffer[length - i - 1] = (i & 0xFF);

	return length;
}

PubSubClient::~PubSubClient() {

}

