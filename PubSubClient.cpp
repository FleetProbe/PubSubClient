#include "PubSubClient.h"
#include <Arduino.h>
#include "libraries/Timer/Timer.h"
#include "libraries/pubsubclient/pubsubconf.h"
#include "libraries/pubsubclient/PubSubClient.h"

#define DEBUG_LOGGING
#define REPORTING_INTERVAL_MS 10000

Timer t;

PubSubClient *client;

void publish_itqos1() {
	client->publish("testpublish", "mytestdataqos1", 0, MQTTQOS1);
}
void publish_itqos0() {
	client->publish("testpublish", "mytestdataqos0", 0, MQTTQOS0);
}

void initMQTT() {
	client->connect("myid", "", "", 0, MQTTQOS1, 0, NULL);
	client->subscribe("testtopic/#", MQTTQOS1);
}

void stopClient() {
//	client.disconnect();
}
void setup() {
	Serial.begin(115200);
	pinMode(27, OUTPUT);
	pinMode(30, OUTPUT);
	digitalWrite(27, HIGH);
	delay(1000);
	digitalWrite(27, LOW);

	client = new PubSubClient(HOST, PORT);

	Serial.println("starting");

	initMQTT();

	t.every(REPORTING_INTERVAL_MS, publish_itqos1);
	t.every(5000, publish_itqos0);
}

int x = 0;

void loop() {
	if(client->connected()){
		client->run();
	}else{
		Serial.println("Disconnected");
		initMQTT();
	}
	delay(200);
	t.update();
}
