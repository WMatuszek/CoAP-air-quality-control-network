/*
 * ConfigMsg.h
 *
 *  Created on: 18.11.2017
 *      Author: Witold
 */

#ifndef RELEASE_CONFIGMSG_H_
#define RELEASE_CONFIGMSG_H_

#include <stdint.h>

class ConfigMsg {

public:
	enum State_t { IDLE = 0, OPEN = 1, COMPLETE = 2 };

	static constexpr const char *ID_TOKEN = "CONFIG";
	static const char SEPARATOR = ',';
	static const char OPEN_CHAR = '$';
	static const char CLOSE_CHAR = '*';

	static const uint8_t MAX_LEN = 100;
	static const uint8_t TOKEN_CNT = 2;
	static const uint8_t TOKEN_SSID = 0;
	static const uint8_t TOKEN_PW = 1;

protected:
	State_t state = IDLE;
	bool valid = false;

	char msg[MAX_LEN];
	uint8_t msgIndex = 0;

	char *tokens[TOKEN_CNT];

public:

	ConfigMsg();
	virtual ~ConfigMsg();

	bool parseCharacter(char c);
	void parseConfigMsg();
	bool isValid();
	bool isComplete();

	void clear();

	char *getMsgBuffer();
	char *getSSID();
	char *getPW();

protected:

	bool tokenCountCorrect();

};

#endif /* RELEASE_CONFIGMSG_H_ */
