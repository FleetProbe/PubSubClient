#include "Sim900Client.h"
char aux_str[MQTT_MAX_PACKET_SIZE];

#ifdef CLIENT_HARDWARE_SERIAL
Sim900Client::Sim900Client(HardwareSerial *_serial) {
#else
	Sim900Client::Sim900Client(SoftwareSerial *_serial) {
#endif
	this->_serial = _serial;

	_serial->begin(9600);

	isConnected = false;
	isModemAwake = false;
	messageNoAnswerCount = 0;
	isGSMinitialized = false;
}

size_t Sim900Client::write(uint8_t *buf, size_t size) {

	int dataacceptmaxlength = 0;
	int z = 0;
	int x = 0;
	int data_accept_found = false;
	char data_accept_read_buffer[MQTT_MAX_PACKET_SIZE];
	memset(data_accept_read_buffer, '\0', MQTT_MAX_PACKET_SIZE);
	char size_buff[10];
	memset(size_buff, '\0', 10);
	//sprintf(aux_str, "AT+CIPSEND=%d\r", size);
	//if (queryconcat(aux_str, 3000, 1, ">")) {

	bool timeout = false;
	unsigned long start_timeout = millis();
	_serial->write("AT+CIPSEND\r");
	while (_serial->read() != '>' && !timeout) {
		if (millis() - start_timeout > 3000) {
			timeout = true;
		}
	}
//	if (query(PSTR2("AT+CIPSEND\r"),3000,2,">","ALREADY CONNECT")) {
	if (!timeout) {
		for (int i = 0; i < size; i++) {
			_serial->write(buf[i]);
		}
		_serial->write(0x1A);
		unsigned long previous = millis();
		do {
			if (!data_accept_found) {
				if (_serial->available() > 0) {
					char c = _serial->read();
					if (c >= ' ') {
						data_accept_read_buffer[z] = c;
						dataacceptmaxlength++;
						z++;
						if (strstr(data_accept_read_buffer, "DATA ACCEPT:") != NULL) {
							char c = '\0';
							do {
								if (_serial->available() > 0) {
									c = _serial->read();
									size_buff[x] = c;
									x++;
								}
							} while (c != 0x0A);
							data_accept_found = true;
							size_buff[x - 2] = '\0';
							if (atoi(size_buff) == size) {
								printPSTR(PSTR2("---message written---"));
								return 1;
							} else {
								printPSTR(PSTR2("---message not written---"));
							}
						}
					}
				}
			}
		} while (millis() - previous < 5000 && dataacceptmaxlength < MQTT_MAX_PACKET_SIZE);
		return 0;
	} else {
		return 0;
	}
}

int Sim900Client::available() {
	return _serial->available();
}

size_t Sim900Client::read(uint8_t *buf, size_t size) {
	unsigned long previous = millis();
	unsigned long len = 0;
	unsigned long message_size = 0UL;
	unsigned long variable_length_size = 0;
	unsigned long topic_length = 0;
	unsigned long topic_start = 256UL;
	bool flashread = false;
	uint32_t oldcrc32 = 0xFFFFFFFF;
	unsigned long timeout = 1UL * 60 * 5000UL;
	unsigned long adress = 0;
	bool id_found = false;
	char c;
	unsigned long id_bytes = 0;
	char topic[MQTT_MAX_TOPIC_SIZE];
	memset(topic, '\0', MQTT_MAX_TOPIC_SIZE);
	char* checksum;
	int x = 0;
	do {
		if (_serial->available() > 0) {
			timeout = millis();
			c = _serial->read();
			if (len < MQTT_MAX_PACKET_SIZE) {
				buf[len] = c;
			}
			len++;
			//Serial.println(String("len") + String(len));
			if (len == 1) {
				message_size = parseVariableLength(&len, buf);
				variable_length_size = len - 1;
				Serial.println(String("message size") + String(message_size));
				Serial.println(String("var length size") + String(variable_length_size));

			} else if (len > variable_length_size + 2 && len < variable_length_size + 4) {
				if (message_size > 2) {
					topic_length = (buf[len - 2] << 8) + buf[len - 1];
					topic_start = len;
					Serial.println(String("topic start") + String(topic_start));
					Serial.println(String("topic size") + String(topic_length));
				}
			} else if (len > topic_start && len < topic_length + variable_length_size + 4) {
				if (c > ' ') {
					Serial.print(c);
				} else {
					char hex[7];
					sprintf(hex, "[%02X]", c);
					Serial.print(hex);
				}
				topic[x++] = c;
			} else if ((buf[0] & 0x06) > 0 && len > topic_start + topic_length && len < topic_start + topic_length + 3) {
				Serial.print("\n");
				if (c > ' ') {
					Serial.print(c);
				} else {
					char hex[7];
					sprintf(hex, "[%02X]", c);
					Serial.print(hex);
				}
				id_found = true;
				id_bytes = 2;
			} else if (len > topic_start + topic_length + id_bytes && ((buf[0] & 0x06) == 0 || id_found)) {
				if (message_size > MQTT_MAX_PACKET_SIZE) {
					flashread = true;
					char hex[7];
					memset(hex, '\0', 7);
					sprintf(hex, "[%02X]", static_cast<unsigned char>(c));
					Serial.print(hex);
					if ((adress + 1) % 16 == 0)
						Serial.println(String("Bytes :  ") + String(adress));
					crc32(oldcrc32, c, adress);
				} else {

				}
			}
		}
	} while (millis() - timeout < 5000 && len < message_size + variable_length_size + 1);

	if (message_size > MQTT_MAX_PACKET_SIZE) {
		strtok(topic, "/");
		strtok(NULL, "/");
		strtok(NULL, "/");
		strtok(NULL, "/");
		checksum = strtok(NULL, "/");
		Serial.print("\n");
		Serial.println(checksum);
		Serial.println(String(~oldcrc32, HEX));
		buf[len + 1] = '\0';
		if (strcmp(String(~oldcrc32, HEX).c_str(), checksum) == 0) {
			return -2;
		}
		return -1;
	}

	buf[len + 1] = '\0';

	return len;
}

int Sim900Client::peek() {
	return 0;
}

void Sim900Client::stop() {
	query(PSTR2("AT+CIPSHUT\r"), 2000, 1, "AT+CIPSHUTSHUT OK");
	isConnected = false;
}
int Sim900Client::connect(const char *host, uint16_t port) {
	if (init()) {
		if (attachGPRS(host, port, 3)) {
			return 1;
		}
	}
	return 0;
}

bool Sim900Client::connected() {
	return isConnected;
}

bool Sim900Client::init() {
	if ((isModemAwake)) { //TODO Check if this is necessary && messageNoAnswerCount < 5)) {
		int counter = 0;
		bool isImsiOK = false;
		while (strlen(imsi) != 15) {
			memset(imsi, '\0', 20);
			getATCommandResponse("AT+CIMI\r", imsi, 1000);
			if (counter > 10) {
				isImsiOK = false;
				break;
			}
			if (strlen(imsi) == 15) {
				isImsiOK = true;
				break;
			} else {
				messageNoAnswerCount++;
			}
			counter++;
		}
		if (strlen(imsi) == 15) {
			isImsiOK = true;
		}
		if (!isGSMinitialized) {
			if (query(PSTR2("AT+CSCLK=1\r"), 3000, 1, "AT+CSCLK=1OK"))
				if (query(PSTR2("AT+CIPQSEND=1\r"), 3000, 1, "AT+CIPQSEND=1OK"))
					if (query(PSTR2("AT+CFUN=1\r"), 2000, 1, "AT+CFUN=1OK"))
						if (query(PSTR2("AT+CBST=0,0,1\r"), 2000, 1, "AT+CBST=0,0,1OK"))
							if (query(PSTR2("AT+IPR=0\r"), 2000, 1, "AT+IPR=0OK")) {
								;
								isGSMinitialized = true;
							}

		}
		if (query(PSTR2("AT\r"), 1000, 1, "ATOK")) {
			return isGSMinitialized && isImsiOK;;
		} else {
			isModemAwake = false;
			return 0;
		}
	} else {
		if (wakeModem()) {
			messageNoAnswerCount = 0;
		}
	}
	return 0;
}

bool Sim900Client::sleepModem() {
	unselect();
#ifdef SDCARD_LOGGING
	printPSTR(PSTR2("Sleep GPRS modem"));
#endif
	return 1;
}
#define EXTENSIVE_LOGGING
int Sim900Client::query(const prog_char *cmd, unsigned int timeout, int nr_of_arguments, ...) {
#ifdef SDCARD_LOGGING
	printPSTR(PSTR2("Modem AT Request  : ")); // Send the AT command
#endif
	printPSTR(cmd); // Send the AT command //TODO get rid of string here
	uint8_t x = 0, answer = 0;

	unsigned long previous;

	delay(100);

	while (_serial->available() > 0)
		_serial->read(); // Clean the input buffer

	va_list arguments;
	va_start(arguments, nr_of_arguments);
	char* placeholder[nr_of_arguments];

	for (int i = 0; i < nr_of_arguments; i++) {
placeholder	[i] = va_arg(arguments,char*);
}
#define MAX_RESPONSE_SIZE strlen(placeholder[0])*2
	char response[MAX_RESPONSE_SIZE];
	memset(response, '\0', MAX_RESPONSE_SIZE); // Initialize the string

	writeSerial(cmd); // Send the AT command

	x = 0;
	previous = millis();

	// this loop waits for the answer
	unsigned char b;
	int z = 0;
	//set watchdog timeout to 2 seconds for reading next serial char
	wdt_enable (WDTO_8S);
	wdt_reset();
	do {

		if (_serial->available() != 0) {
			b = _serial->read();
			if (b >= ' ') {
				response[x++] = b;
#ifdef EXTENSIVE_LOGGING
				Serial.print(static_cast<char>(b));
#endif
				if (x == MAX_RESPONSE_SIZE) {
					memset(response, '\0', MAX_RESPONSE_SIZE);
					x = 0;
				}
			}
			// check if the desired answer is in the response of the module
			while (z < nr_of_arguments) {
				if (strstr(response, placeholder[z]) != NULL) {
					answer = 1;
				}
				z++;
			}
		}
		// Waits for the asnwer with time out
		z = 0;
	} while ((answer == 0) && ((millis() - previous) < timeout));

	wdt_disable();
#ifdef EXTENSIVE_LOGGING
	Serial.print('\n');
#endif

#ifdef SDCARD_LOGGING
	printPSTR(PSTR2("Modem AT Response : "));
#endif
	if (answer) {
		Serial.println(response);
		messageNoAnswerCount = 0;
		return 1;
	} else {
#ifdef SDCARD_LOGGING
		printPSTR(PSTR2("Answer not found"));
#endif
		messageNoAnswerCount++;
		return answer;
	}

}

void Sim900Client::getATCommandResponse(const char *cmd, char* response, unsigned int timeout) {
	uint8_t x = 0;
	unsigned long previous;

	delay(100);

	while (_serial->available() > 0)
		_serial->read(); // Clean the input buffer

	_serial->println(cmd); // Send the AT command
#ifdef SDCARD_LOGGING
			printPSTR(PSTR2("Modem AT Request  : ")); // Send the AT command
#endif
	Serial.print(String(cmd)); // Send the AT command //TODO get rid of string here
	x = 0;
	previous = millis();

	int row = 0;

	// we're interested in the second row only
	int responseRow = 2;

	// this loop waits for the answer
	wdt_enable (WDTO_8S);
	wdt_reset();
	do {
		//set watchdog timeout to 2 seconds for reading next serial char
		// if there are data in the UART input buffer, reads it and checks for the asnwer
		// LF  Line feed        '\n'    0x0A    10
		// CR Carriage return   '\r'    0x0D    13
		if (_serial->available() != 0) {

			char b = _serial->read();
			//Serial.print(b);
			if (b == '\n') {
				row++;
			}

			//Serial.println("[" + String(b) + "] = " + String(b>=' ') + " - " + String(b=='\r') + " - " + String(b=='\n') + " - " + String(row));
			if (b >= ' ' && row == responseRow) {
				if (x < 15) {
					response[x] = b;
					x++;
				}
			}
		}
	}
	// Waits for the asnwer with time out
	while (((millis() - previous) < timeout));
	wdt_disable();

#ifdef SDCARD_LOGGING
	printPSTR(PSTR2("Modem AT Response : "));
#endif
	Serial.print(response);

}

#define GPRS_APN "gprs.base.be"
#define GPRS_LOGIN "base"
#define GPRS_PASSWORD "base"

bool Sim900Client::attachGPRS(const char* host, uint16_t port, int retries) {
	sprintf(aux_str, "AT+CIPSTART=\"TCP\",\"%s\",%d", host, port);

#ifdef SDCARD_LOGGING
	printPSTR(PSTR2(" +++++++++++++++ start attach gprs +++++++++++++++"));
#endif
	int i = 0;

//retry 200000 times when value = -1
	if (retries == -1)
		i = -32000;

//	int size = 128;
//	char attachstring[size];
//	memset(attachstring, '\0', size);
//	sprintf(attachstring, "AT+CSTT=\"%s\",\"%s\",\"%s\"\r\0", apn, user, pwd);

	String attachstring = String("AT+CSTT=\"") + String(GPRS_APN) + String("\",\"") + String(GPRS_LOGIN) + String("\",\"") + String(GPRS_PASSWORD) + String("\"\r");

	while (i < retries) {
		int rpm = 0;
		char result[128];
		memset(result, '\0', 128);

		bool isNetworkRegistration = false;

		if (init()) {
			if (query(PSTR2("AT+CIPSHUT\r"), 2000, 1, "AT+CIPSHUTSHUT OK")) {
				if (query(PSTR2("AT+CREG?\r"), 2000, 1, "AT+CREG?+CREG: 0,1")) {
					if (query(PSTR2("AT+CIPMUX=0\r"), 2000, 1, "AT+CIPMUX=0OK"))
						isNetworkRegistration = true;
				}
			}
		}

		bool isGPRSAttached = false;
		if (isNetworkRegistration) {
			if (query(PSTR2("AT+CIPSTATUS\r"), 2000, 1, "AT+CIPSTATUSOKSTATE: IP INITIAL"))
				if (queryconcat(attachstring, 2000, 1, "OK"))
					if (query(PSTR2("AT+CIPSTATUS\r"), 2000, 1, "AT+CIPSTATUSOKSTATE: IP START"))
						if (query(PSTR2("AT+CIICR\r"), 6000, 1, "AT+CIICROK"))
							if (query(PSTR2("AT+CIPSTATUS\r"), 2000, 1, "AT+CIPSTATUSOKSTATE: IP GPRSACT"))
								if (query(PSTR2("AT+CIFSR\r"), 2000, 1, "."))
									if (query(PSTR2("AT+CIPSTATUS\r"), 2000, 1, "AT+CIPSTATUSOKSTATE: IP STATUS"))
										if (queryconcat(aux_str, 10000, 1, "CONNECT OK"))
											isGPRSAttached = true;
		}

		if (isGPRSAttached && isNetworkRegistration) {
#ifdef SDCARD_LOGGING
			printPSTR(PSTR2(" +++++++++++++++  stop attach gprs true +++++++++++++++ "));
#endif
			isConnected = true;
			return 1;

		}
		i++;
	}
	printPSTR(PSTR2(" +++++++++++++++  stop attach gprs false +++++++++++++++ "));
	return 0;
}
char* Sim900Client::getImsi() {
	return imsi;
}
bool Sim900Client::wakeModem() {
	select();
	int counter = 0;
	while (!query(PSTR2("ATZ\r"), 1000, 1, "ATZOK")) {
		if (counter > 2) { // TODO check retries
			poweron();
			return 0;
		}
		isGSMinitialized = false;
		poweron();
		select();
		counter++;
	}
	isModemAwake = true;
	return 1;
}
Sim900Client::operator bool() {
}

int Sim900Client::queryconcat(String cmd, unsigned int timeout, int nr_of_arguments, ...) {

#ifdef SDCARD_LOGGING
	printPSTR(PSTR2("Modem AT Request  : ")); // Send the AT command
#endif
	Serial.print(cmd); // Send the AT command //TODO get rid of string here

	uint8_t x = 0, answer = 0;
	String response = "";
//		memset(response, '\0', 1024);

	unsigned long previous;

	delay(100);

	while (_serial->available() > 0)
		_serial->read(); // Clean the input buffer

	va_list arguments;
	va_start(arguments, nr_of_arguments);
	char* placeholder[nr_of_arguments];

	for (int i = 0; i < nr_of_arguments; i++) {
placeholder	[i] = va_arg(arguments,char*);
}

	_serial->println(cmd); // Send the AT command

	x = 0;
	previous = millis();

// this loop waits for the answer
	unsigned char b;
	int z = 0;
//set watchdog timeout to 2 seconds for reading next serial char
	wdt_enable (WDTO_8S);
	do {
		wdt_reset();
		if (_serial->available() != 0) {
			b = _serial->read();
			if (b >= ' ') {
				response.concat((static_cast<char>(b)));
#ifdef EXTENSIVE_LOGGING
				Serial.print(static_cast<char>(b));
#endif
				if (response.length() > 128)
					return false;
			}
			// check if the desired answer is in the response of the module
			while (z < nr_of_arguments) {
				if (strstr(response.c_str(), placeholder[z]) != NULL) {
					answer = 1;
				}
				z++;
			}
		}
		z = 0;
	} while ((answer == 0) && ((millis() - previous) < timeout));

	if (answer == 1) {
#ifdef EXTENSIVE_LOGGING
		Serial.print("\n");
#endif
		wdt_disable();
#ifdef SDCARD_LOGGING
		printPSTR(PSTR2("Modem AT Response : "));
#endif
		printPSTR(PSTR2("response ok"));
		return 1;
	} else {
#ifdef SDCARD_LOGGING
		printPSTR(PSTR2("Answer not found"));
#endif

		return 0;
	}
}
