/*
 * CoAP_node.h
 *
 *  Created on: 13.11.2017
 *      Author: Witold
 */

#ifndef COAP_NODE_H_
#define COAP_NODE_H_

#include "NodeConfig.h"

#define SEC_TO_MS(s)								(1e3*(s))
#define SEC_TO_US(s)								(1e6*(s))

enum SType_t { NONE = 0, AIRQ, TEMP, PRESS, TEMP_PRESS };
enum SState_t { OFF = 0, ON = 1 };
enum SMeasureMode_t { CONTINOUS = 0, ON_DEMAND = 1};

#define WIFI_CONNECT_ON_STARTUP
#define NO_LED_BLINK

#define _SERIAL_CONSOLE								Serial

static const uint8_t HKA5_POWER_CTRL_PIN 			= 0; // D3

static const uint32_t SERVER_REPORT_INTERVAL_SEC 	= 20;
static const uint32_t WIFI_CONNECT_WAIT_SEC 		= 15;
static const uint32_t MEASUREMENT_CACHE_SIZE 		= 1;

static const char RESPONSE_OK[] 					= "OK";
static const char RESPONSE_FAIL[] 					= "FAIL";
static const char RESPONSE_NOT_SUPPORTED[] 			= "SETTING NOT SUPPORTED";

#endif /* COAP_NODE_H_ */
