#ifndef Sim900Client_h
#define Sim900Client_h

#include <Stream.h>
#include <Arduino.h>
#include <avr/wdt.h>
#include "../../pubsubconf.h"
#include "../../ClientUtilities.h"
#include "../../Checksum.h"

#define SIM900_SLOW_CLOCK_PIN	27
#define SIM900_POWER_ON_PIN		30

class Sim900Client {
private:
#ifdef CLIENT_HARDWARE_SERIAL
	HardwareSerial *_serial;
#else
	SoftwareSerial *_serial;
#endif
	boolean isConnected;
	void writeSerial(const prog_char *data) {
		while (pgm_read_byte(data) != 0x00)
			_serial->write((char) pgm_read_byte(data++));
	}
	char imsi[20];
	bool isGSMinitialized;
	bool isModemAwake;
	int messageNoAnswerCount;
	void getATCommandResponse(const char *cmd, char *response, unsigned int timeout);
	int queryconcat(String cmd, unsigned int timeout, int nr_ofarguments, ...);
public:

#ifdef CLIENT_HARDWARE_SERIAL
	Sim900Client(HardwareSerial *_serial);
#else
	Sim900Client(SoftwareSerial *_serial);
#endif
	char* getImsi();
	bool init();
	bool attachGPRS(const char* host, uint16_t port, int retries);
	int connect(const char *host, uint16_t port);
	size_t write(uint8_t *buf, size_t sizeh);
	int available();
	size_t read(uint8_t *buf, size_t size);
	int peek();
	void stop();
	void flush() {
		while (_serial->available() > 0) {
			_serial->read();
		}
	}
	bool connected();
	operator bool();
	bool sleepModem();
	bool wakeModem();
	bool readResult(char result[]);
	int query(const prog_char *cmd, unsigned int timeout, int nr_ofarguments, ...);

    void select(){
    	digitalWrite(SIM900_SLOW_CLOCK_PIN, LOW);
    }

    void unselect(){
    	digitalWrite(SIM900_SLOW_CLOCK_PIN, HIGH);
    }

    void poweron(){
    	digitalWrite(SIM900_SLOW_CLOCK_PIN, HIGH);
    	pinMode(SIM900_POWER_ON_PIN, OUTPUT);
		digitalWrite(SIM900_POWER_ON_PIN, HIGH);
		delay(1200);
		digitalWrite(SIM900_POWER_ON_PIN, LOW);
		delay(2000);
		digitalWrite(SIM900_POWER_ON_PIN, HIGH);
    }

	unsigned long parseVariableLength(unsigned long *counter, uint8_t* buf) {
		unsigned long result = 0UL;
		unsigned long multiplier = 1UL;
		do {
			if (_serial->available() > 0) {
				uint8_t c = _serial->read();
				buf[(*counter)++] = c;
				result += (c & 0x7F) * multiplier;
				multiplier *= 128;
				if (c >> 7 == 0) {
					return result;
				}
			}
			//TODO NOT hardcode length here
		} while (*counter < 256);
		return -1;
	}
};

#endif
