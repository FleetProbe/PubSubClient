Arduino SubPubClient(MQTT).

Clients for connecting to internet:

	Sim900

Supports :

	Publish(QOS0 and QOS1)
	Subscriptions(QOS0 and QOS1)
	QOS2 not supported due to restrictions memory wise on arduino
	

Follow the eclipse-arduino-setup-osx.md to be able to compile and deploy this project.

Settings should be adjusted according to your mqtt broker and your sim card provider in :

	libraries/pubsubclient/pubsubconf.h
	
Currently only working with hardwareserial this should be set in :
	
	libraries/pubsubclient/PubSubClient.cpp line16 (currently Serial2)

You can use the following link for testing purposes :

	http://www.hivemq.com/demos/websocket-client/