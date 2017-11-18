/*
 * ConfigMsg.cpp
 *
 *  Created on: 18.11.2017
 *      Author: Witold
 */

#include "ConfigMsg.h"

#include <cstring>
#include "Arduino.h"

ConfigMsg::ConfigMsg() {
	for (int i=0; i<ConfigMsg::TOKEN_CNT; ++i)
		tokens[i] = nullptr;
}

ConfigMsg::~ConfigMsg() {}

bool ConfigMsg::parseCharacter(char c) {

	if (msgIndex > ConfigMsg::MAX_LEN) return false;

	switch (state) {
	case State_t::IDLE:
		if (c != ConfigMsg::OPEN_CHAR) return false;
		msg[msgIndex++] = c;
		state = State_t::OPEN;
		break;

	case State_t::OPEN:
		msg[msgIndex++] = c;
		if (c == ConfigMsg::CLOSE_CHAR) {
			msg[msgIndex - 1] = '\0';
			state = State_t::COMPLETE;
		}
		break;

	case State_t::COMPLETE:
		return false;
		break;
	}

	return true;
}

void ConfigMsg::parseConfigMsg() {
	Serial.println("Config message parse:");

	if (!isComplete()) {
		valid = false;
		return;
	}

	Serial.println("- complete");

	if (!tokenCountCorrect()) {
		valid = false;
		return;
	}
	Serial.println("- token cnt ok");

	char sep[2] = { ConfigMsg::SEPARATOR, '\0' };
	char *tmp = msg + 1;
	char *id = strtok(tmp, sep);

	if (strcmp(id, ID_TOKEN) != 0){
		valid = false;
		return;
	}

	Serial.println("- id token ok");

	tokens[ConfigMsg::TOKEN_SSID] = strtok(NULL, sep);
	tokens[ConfigMsg::TOKEN_PW] = strtok(NULL, sep);

	valid = true;
}

bool ConfigMsg::isValid() {
	return valid;
}

bool ConfigMsg::isComplete() {
	return state == State_t::COMPLETE;
}

void ConfigMsg::clear() {
	state = IDLE;
	valid = false;
	msgIndex = 0;
}

char* ConfigMsg::getMsgBuffer() {
	return msg;
}

char* ConfigMsg::getSSID() {
	return tokens[ConfigMsg::TOKEN_SSID];
}

char* ConfigMsg::getPW() {
	return tokens[ConfigMsg::TOKEN_PW];
}

bool ConfigMsg::tokenCountCorrect() {
	uint8_t sepCnt = 0;
	char *tmp = strchr(msg, ConfigMsg::SEPARATOR);
	while (tmp != NULL){
		sepCnt += 1;
		tmp = strchr(tmp+1, ConfigMsg::SEPARATOR);
	}
	return (sepCnt == ConfigMsg::TOKEN_CNT);
}
