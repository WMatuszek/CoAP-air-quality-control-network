#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2017-11-16 02:37:02

#include "Arduino.h"
#include "CoAP_node.h"
#include "Arduino.h"
#include "SPI.h"
#include "WiFi.h"
#include "WiFiUdp.h"
#include "coap.h"
#include "HKA5Controller.h"
#include "BMP280Controller.h"
void COAP_callback_PM(CoapPacket &packet, IPAddress ip, int port) ;
void COAP_callback_pressure(CoapPacket &packet, IPAddress ip, int port) ;
void COAP_callback_nodeInfo(CoapPacket &packet, IPAddress ip, int port) ;
bool serialCheck(void);
bool readConfigMsg(void);
bool readPM(void) ;
void printPM(void);
void setup() ;
void loop() ;
void COAP_callback_response(CoapPacket &packet, IPAddress ip, int port) ;


#include "CoAP_node.ino"

#endif
