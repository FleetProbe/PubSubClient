#ifndef CLIENTUTIL_H
#define CLIENTUTIL_H
#include "pubsubconf.h"
#include "../memory/MemoryFree.h"

inline void printPSTR(const prog_char *data) {
	Serial.print(String(freeMemory()));
	Serial.print(" ");
	while (pgm_read_byte(data) != 0x00) {
		Serial.print((char) pgm_read_byte(data++));
	}
	Serial.print('\n');
}


#endif
