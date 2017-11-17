#include "CoAP_node.h"

#include "Arduino.h"
#include "Wire.h"
#include "WiFi.h"
#include "WiFiUdp.h"

#include "coap.h"

#include "QueueArray.h"

#include "HKA5Controller.h"
#include "BMP280Controller.h"


char ssid[10]    		= "SSID";
const char* password 	= "PW";

static uint8_t HKA5_msgBuffer[HKA5::MSG::LENGTH];

//static QueueArray<uint16_t> PM_1_queue;

static uint16_t PM_1;
static uint16_t PM_2_5;
static uint16_t PM_10;

/*
 * GLobal variables
 */

// UDP and CoAP class
WiFiUDP udp;
Coap coap(udp);

BMP280::BMP280Controller BMP280Ctrl(&Wire);
HKA5::HKA5Controller HKA5Ctrl;

/*
 * CoAP resource access callbacks
 */

void COAP_callback_PM(CoapPacket &packet, IPAddress ip, int port) {}
void COAP_callback_pressure(CoapPacket &packet, IPAddress ip, int port) {}
void COAP_callback_nodeInfo(CoapPacket &packet, IPAddress ip, int port) {}
void COAP_callback_response(CoapPacket &packet, IPAddress ip, int port);

void setup() {
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	delay(2000);

	_SERIAL_CONSOLE.begin(9600);

	_SERIAL_CONSOLE.println("--- Config begin");

	_SERIAL_CONSOLE.println("HKA5 sensor config");
	_SERIAL_HKA5.begin(HKA5::USART_BAUD_RATE);
	_SERIAL_HKA5.setTimeout(HKA5::USART_TIMEOUT_MS);
	HKA5Ctrl.attachMessagePtr(HKA5_msgBuffer);

	_SERIAL_CONSOLE.print("BMP280 sensor config ");
	for (int i = 0; i < 3; ++i){
		delay(200);
		if (BMP280Ctrl.begin()) {
			_SERIAL_CONSOLE.print(" OK");
			break;
		}
		_SERIAL_CONSOLE.print(" FAIL");
	}

	_SERIAL_CONSOLE.println("\r\nWIFI config");

//	if (WiFi.begin(ssid, password) == WL_CONNECT_FAILED)
//		_SERIAL_CONSOLE.print("begin() failed");
//
//
////	while (WiFi.status() != WL_CONNECTED) {
////		delay(500);
////		Serial.print(".");
////	}
//// _SERIAL_CONSOLE.println(" CONNECTED!");
//
//
//	_SERIAL_CONSOLE.println("CoAP config");
//
//	coap.server(COAP_callback_PM, "PM");
//	coap.server(COAP_callback_pressure, "pressure");
//	coap.server(COAP_callback_nodeInfo, "nodeInfo");
//	coap.response(COAP_callback_response);
//	coap.start();
//
//	_SERIAL_CONSOLE.println("--- Config done");

}

void loop() {
	digitalWrite(LED_BUILTIN, HIGH);
	delay(500);
	digitalWrite(LED_BUILTIN, LOW);
	delay(500);

	serialCheck();
	printPM();

	float tmp = BMP280Ctrl.readTemperature();
	_SERIAL_CONSOLE.println(tmp);
	float press = BMP280Ctrl.readPressure();
	_SERIAL_CONSOLE.println(press);
}

/*
 *  -----------------------------------------------------------------------
 */

bool serialCheck(void){
	while (_SERIAL_HKA5.available()){
		uint8_t token = _SERIAL_HKA5.read();
		if (token == HKA5::MSG::OPEN_TOKEN)
			readPM();
		if (token == CONFIG_MSG::OPEN_TOKEN)
			readConfigMsg();
	}
	return false;
}

bool readConfigMsg(void){
	// #TODO unpack config NMEA style string
	// Update ssid, PW, others?
	// Reinit WIFI & COAP
	return false;
}

bool readPM(void) {
	if (HKA5::MSG::LENGTH
			== _SERIAL_HKA5.readBytes(HKA5_msgBuffer, HKA5::MSG::LENGTH)) {
		PM_1 = HKA5Ctrl.getPM_1();
		PM_2_5 = HKA5Ctrl.getPM_2_5();
		PM_10 = HKA5Ctrl.getPM_10();
		return true;
	}
	return false;
}

void printPM(void){
	_SERIAL_CONSOLE.println("PM values [ug/m3]:");
	_SERIAL_CONSOLE.print("PM 1 = ");
	_SERIAL_CONSOLE.println(PM_1);
	_SERIAL_CONSOLE.print("PM 2.5 = ");
	_SERIAL_CONSOLE.println(PM_2_5);
	_SERIAL_CONSOLE.print("PM 10 = ");
	_SERIAL_CONSOLE.println(PM_10);
}




//// CoAP server endpoint URL
//void callback_light(CoapPacket &packet, IPAddress ip, int port) {
//  Serial.println("[Light] ON/OFF");
//
//  // send response
//  char p[packet.payloadlen + 1];
//  memcpy(p, packet.payload, packet.payloadlen);
//  p[packet.payloadlen] = NULL;
//
//  String message(p);
//
//  if (message.equals("0"))
//    LEDSTATE = false;
//  else if(message.equals("1"))
//    LEDSTATE = true;
//
//  if (LEDSTATE) {
//    digitalWrite(9, HIGH) ;
//    coap.sendResponse(ip, port, packet.messageid, "1");
//  } else {
//    digitalWrite(9, LOW) ;
//    coap.sendResponse(ip, port, packet.messageid, "0");
//  }
//}
//

// CoAP client response callback
void COAP_callback_response(CoapPacket &packet, IPAddress ip, int port) {
  _SERIAL_CONSOLE.println("[Coap Response got]");
  (void)port;
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;

  _SERIAL_CONSOLE.println(p);
}
