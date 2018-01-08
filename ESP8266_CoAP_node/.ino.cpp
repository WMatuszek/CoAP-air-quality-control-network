#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2018-01-08 12:28:57

#include "Arduino.h"
#include "CoAP_node.h"
#include "Arduino.h"
#include "Wire.h"
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <ArduinoJson-v5.12.0.hpp>
#include "coap_server.h"
#include "HKA5Controller.h"
#include "BMP280Controller.h"
#include "ConfigMsg.h"
void setup() ;
void SensorsSetup() ;
void WiFiSetup();
void CoAPSetup();
void loop() ;
void sleepFor(uint16_t sleep_s) ;
bool USARTSerialInputCheck(void);
bool ReadNodeConfigMsg(void);
bool ReadPM(void) ;
void PrintPM(void);
uint32_t GetBatteryStatus(void) ;
uint16_t CoAP_Ping(IPAddress ip, uint16_t port) ;
uint16_t CoAP_NodeReportToServer() ;
void COAP_callback_PM(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_pressure(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_temperature(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_battery(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_nodeInfo(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_sleepCycle(coapPacket *packet, IPAddress ip, int port, int observer);


#include "CoAP_node.ino"

#endif
