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

struct NodeConfig {
	uint8_t savedToEEPROM = 0;
	bool sleepCycleActive;
	uint16_t sleepTime_s;
	uint16_t measureTime_s;

	char nodeName[NODE_NAME_BUFFER_SIZE];
	char ssid[WIFI_CRED_BUFFER_SIZE];
	char password[WIFI_CRED_BUFFER_SIZE];

	NodeConfig() {
		EEPROM.begin(512);
		NodeConfig *thisPtr = this;
		EEPROM.get(0, thisPtr);
		if (savedToEEPROM == 0) { // No config in EEPROM
			setDefault();
		}
	}

	void setDefault() {
		sleepCycleActive 					= false;
		sleepTime_s							= 0;
		measureTime_s 						= 0;
		strcpy(nodeName, 					"default");
		strcpy(ssid, 						"OpiAP");
		strcpy(password, 					"herpderp");
	}

	void saveToEEPROM(){
		savedToEEPROM = 1;
		EEPROM.put(0, this);
	}

};

#endif /* NODECONFIG_H_ */
