/*
 * NodeConfig.h
 *
 *  Created on: 03.01.2018
 *      Author: Witold
 */

#ifndef NODECONFIG_H_
#define NODECONFIG_H_

#include "EEPROM.h"

#define NODE_NAME_BUFFER_SIZE			20
#define WIFI_CRED_BUFFER_SIZE			30

static const uint32_t 	EEPROM_SIZE = 512;
static const uint8_t 	EEPROM_WRITTEN = 0x1A;
static const uint16_t 	NODE_CONFIG_SIZE = 1 + 2*2 + NODE_NAME_BUFFER_SIZE + 2*WIFI_CRED_BUFFER_SIZE;

struct NodeConfig {
	uint8_t savedToEEPROM = 0;
	uint8_t sleepCycleActive;
	uint16_t sleepTime_s;
	uint16_t measureTime_s;

	char nodeName[NODE_NAME_BUFFER_SIZE];
	char ssid[WIFI_CRED_BUFFER_SIZE];
	char password[WIFI_CRED_BUFFER_SIZE];

	void setDefault() {
		sleepCycleActive 					= 0;
		sleepTime_s							= 0;
		measureTime_s 						= 0;
		strcpy(nodeName, 					"default");
		strcpy(ssid, 						"OpiAP");
		strcpy(password, 					"herpderp");
	}


	static void LoadFromEEPROM(NodeConfig *config) {
		EEPROM.begin(EEPROM_SIZE);
		if (EEPROM.read(0) == EEPROM_WRITTEN) {

			uint8_t * byteStorageRead = (uint8_t *) config;
			for (size_t i = 0; i < NODE_CONFIG_SIZE; i++) {
				byteStorageRead[i] = EEPROM.read(0 + i);
			}
			Serial.println("Settings loaded from EEPROM");
		}
		EEPROM.end();
	}

	static void SaveToEEPROM(NodeConfig *config) {
		Serial.print("Writing settings to EEPROM");
		config->savedToEEPROM = EEPROM_WRITTEN;

		uint8_t old = config->sleepCycleActive;
		config->sleepCycleActive = 0; // No save pls #TODO remove

		EEPROM.begin(EEPROM_SIZE);

		uint8_t * byteStorage = (uint8_t *) config;
		for (size_t i = 0; i < NODE_CONFIG_SIZE; i++) {
			EEPROM.write(0 + i, byteStorage[i]);
		}

		EEPROM.commit();
		EEPROM.end();

		config->sleepCycleActive = old; // Revert
	}

};

#endif /* NODECONFIG_H_ */
