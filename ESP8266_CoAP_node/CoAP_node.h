/*
 * CoAP_node.h
 *
 *  Created on: 13.11.2017
 *      Author: Witold
 */

#ifndef COAP_NODE_H_
#define COAP_NODE_H_

#include "NodeConfig.h"

enum Sensor_t { NONE = 0, AIRQ, TEMP, PRESS, TEMP_PRESS };
enum SensorState_t { ON = 1, OFF = 0 };
enum MeasureMode_t { CONTINOUS = 0, ON_DEMAND };

#define WIFI_CONNECT_ON_STARTUP

#define _SERIAL_CONSOLE								Serial

static const uint8_t HKA5_POWER_CTRL_PIN 			= 0;

static const uint32_t MEASUREMENT_CACHE_SIZE 		= 1;

static const float TEMP_MIN_DELTA					= 0.1;
static const float PRESS_MIN_DELTA					= 0.1;
static const uint16_t PM_MIN_DELTA					= 1;

static const char RESPONSE_OK[] = "OK";
static const char RESPONSE_FAIL[] = "FAIL";

#endif /* COAP_NODE_H_ */
