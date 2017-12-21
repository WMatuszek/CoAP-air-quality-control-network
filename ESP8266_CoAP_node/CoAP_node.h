/*
 * CoAP_node.h
 *
 *  Created on: 13.11.2017
 *      Author: Witold
 */

#ifndef COAP_NODE_H_
#define COAP_NODE_H_

enum Sensor_t { NONE = 0, AIRQ, TEMP, PRESS };

#define WIFI_CONNECT_ON_STARTUP

#define _SERIAL_CONSOLE								Serial

static const uint32_t WIFI_CRED_BUFFER_SIZE			= 30;
static const uint32_t MEASUREMENT_CACHE_SIZE 		= 1;

static const float TEMP_MIN_DELTA					= 0.1;
static const float PRESS_MIN_DELTA					= 0.1;
static const uint16_t PM_MIN_DELTA					= 1;

static const char SERVER_REPORT_URI[] = "node_report";

#endif /* COAP_NODE_H_ */
