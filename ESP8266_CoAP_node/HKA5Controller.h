/*
 * HKA5Controller.h
 *
 *  Created on: 06.11.2017
 *      Author: Witold
 */

#ifndef HKA5CONTROLLER_H_
#define HKA5CONTROLLER_H_

#include <stdint.h>

namespace HKA5 {

struct PMData_t {
	uint16_t PM_1;
	uint16_t PM_2_5;
	uint16_t PM_10;
};

const uint32_t USART_BAUD_RATE = 9600;
const uint32_t USART_TIMEOUT_MS = 200;

namespace MSG {
	const uint8_t OPEN_TOKEN = 0x42;
	const uint8_t LENGTH = 31;   //0x42 + 31 bytes
	const uint8_t PM_1_MSB = 3;
	const uint8_t PM_1_LSB = 4;
	const uint8_t PM_2_5_MSB = 5;
	const uint8_t PM_2_5_LSB = 6;
	const uint8_t PM_10_MSB = 7;
	const uint8_t PM_10_LSB = 8;
}

class HKA5Controller {

protected:

	uint8_t *msg;

	uint8_t pin_sleep_ctrl;

public:
	HKA5Controller();
	virtual ~HKA5Controller();

	uint8_t *getMessagePtr(void) { return msg; }
	void attachMessagePtr(uint8_t *msgPtr){ msg = msgPtr; }

	PMData_t getPM(void){
		return {getPM_1(), getPM_2_5(), getPM_10()};
	}

	uint16_t getPM_1(void) {
		uint8_t msb = *(msg + MSG::PM_1_MSB);
		uint8_t lsb = *(msg + MSG::PM_1_LSB);
		return ((uint16_t)msb << 8) | lsb;
	}
	uint16_t getPM_2_5(void){
		uint8_t msb = *(msg + MSG::PM_2_5_MSB);
		uint8_t lsb = *(msg + MSG::PM_2_5_LSB);
		return ((uint16_t)msb << 8) | lsb;
	}
	uint16_t getPM_10(void){
		uint8_t msb = *(msg + MSG::PM_10_MSB);
		uint8_t lsb = *(msg + MSG::PM_10_LSB);
		return ((uint16_t)msb << 8) | lsb;
	}

	void sleep(void) {} // set pin high
	void wakeUp(void) {} // set pin low
};

} /* namespace HKA5 */

#endif /* HKA5CONTROLLER_H_ */
