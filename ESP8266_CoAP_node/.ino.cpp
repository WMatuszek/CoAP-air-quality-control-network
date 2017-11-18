#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2017-11-18 19:05:40

#include "Arduino.h"
#include "CoAP_node.h"
#include "Arduino.h"
#include "Wire.h"
#include <ESP8266WiFi.h>
#include <coap_server.h>
#include "QueueArray.h"
#include "HKA5Controller.h"
#include "BMP280Controller.h"
#include "ConfigMsg.h"
void setup() ;
void setup_sensors() ;
void setup_wifi();
void setup_coap();
void loop() ;
bool USARTSerialInputCheck(void);
bool readConfigMsg(void);
bool readPM(void) ;
void printPM(void);
void COAP_callback_PM(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_pressure(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_temperature(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_nodeInfo(coapPacket *packet, IPAddress ip, int port, int observer);


#include "CoAP_node.ino"

#endif
