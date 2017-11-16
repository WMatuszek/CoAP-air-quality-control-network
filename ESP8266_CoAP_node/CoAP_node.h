/*
 * CoAP_node.h
 *
 *  Created on: 13.11.2017
 *      Author: Witold
 */

#ifndef COAP_NODE_H_
#define COAP_NODE_H_

#define _SERIAL_HKA5		Serial
#define _SERIAL_CONSOLE		Serial

#define PIN_BMP280_CS		SS

// #TODO NMEA style config of SSID, PASSWORD
namespace CONFIG_MSG {
	const uint8_t OPEN_TOKEN = 0x22;

	const uint8_t LENGTH = 10;
}

#endif /* COAP_NODE_H_ */
