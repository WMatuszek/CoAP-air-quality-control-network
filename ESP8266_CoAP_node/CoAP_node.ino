#include "CoAP_node.h"

#include "Arduino.h"
#include "Wire.h"
#include <ESP8266WiFi.h>
#include <coap_server.h>
#include "QueueArray.h"

#include "HKA5Controller.h"
#include "BMP280Controller.h"
#include "ConfigMsg.h"

static char ssid[WIFI_CRED_BUFFER_SIZE]			= "OpiAP";
static char password[WIFI_CRED_BUFFER_SIZE] 	= "herpderp";

static char node_info_msg[50] 					= "Node info here";

coapServer coap;

/*
 * GLobal variables
 */
static uint8_t HKA5_msgBuffer[HKA5::MSG::LENGTH];

//static QueueArray<uint16_t> PM_1_queue;
static uint16_t PM_1;
static uint16_t PM_2_5;
static uint16_t PM_10;


BMP280::BMP280Controller BMP280Ctrl(&Wire);
HKA5::HKA5Controller HKA5Ctrl;

ConfigMsg configMsg;

/*
 * CoAP resource access callbacks
 */
void COAP_callback_PM(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_pressure(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_temperature(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_nodeInfo(coapPacket *packet, IPAddress ip, int port, int observer);
//void COAP_callback_response(coapPacket *packet, IPAddress ip, int port, int observer) {}

void setup() {
	yield();

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	delay(2000);

	yield();

	_SERIAL_CONSOLE.begin(HKA5::USART_BAUD_RATE);

	setup_sensors();

	yield();

#ifdef WIFI_CONNECT_ON_STARTUP
	setup_wifi();
	yield();
	setup_coap();
#endif

	_SERIAL_CONSOLE.println("--- Config done");

}

void setup_sensors() {
	// HKA5 config
	_SERIAL_CONSOLE.println("\r\n\r\nHKA5 sensor config");
	_SERIAL_CONSOLE.setTimeout(HKA5::USART_TIMEOUT_MS);
	HKA5Ctrl.attachMessagePtr(HKA5_msgBuffer);

	yield();

	// BMP280 config
	_SERIAL_CONSOLE.print("BMP280 sensor config ");
	for (int i = 0; i < 3; ++i) {
		delay(200);
		if (BMP280Ctrl.begin()) {
			_SERIAL_CONSOLE.print(" OK");
			break;
		}
		_SERIAL_CONSOLE.print(" FAIL");
	}
}

void setup_wifi(){
	// WIFI config
	_SERIAL_CONSOLE.println("\r\nWIFI config");
	WiFi.setAutoConnect(false);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500); yield();
		Serial.print(".");
	}
	_SERIAL_CONSOLE.println(" CONNECTED!");
	_SERIAL_CONSOLE.println(WiFi.localIP());
}

void setup_coap(){
	// CoAP config
	_SERIAL_CONSOLE.println("CoAP config");
	coap.server(COAP_callback_PM, "PM");
	coap.server(COAP_callback_pressure, "pressure");
	coap.server(COAP_callback_temperature, "temperature");
	coap.server(COAP_callback_nodeInfo, "info");
//	coap.server(COAP_callback_response, "resp");
	coap.start();
}

void loop() {
	yield();
	coap.loop();

	digitalWrite(LED_BUILTIN, HIGH);
	delay(500);

	yield();
	coap.loop();

	digitalWrite(LED_BUILTIN, LOW);
	delay(500);

	USARTSerialInputCheck();

	if (WiFi.isConnected() == false){
		_SERIAL_CONSOLE.println("WIFI disconnected, attempting to reconnect");
		WiFi.reconnect();
	}

//	printPM();
//	float tmp = BMP280Ctrl.readTemperature();
//	_SERIAL_CONSOLE.println(tmp);
//	float press = BMP280Ctrl.readPressure();
//	_SERIAL_CONSOLE.println(press);
}

/*
 *  -----------------------------------------------------------------------
 */

bool USARTSerialInputCheck(void){
	while (_SERIAL_CONSOLE.available()){
		uint8_t token = _SERIAL_CONSOLE.read();
		if (token == HKA5::MSG::OPEN_TOKEN)
			readPM();
		if (token == ConfigMsg::OPEN_CHAR)
			readConfigMsg();
	}
	return false;
}

bool readConfigMsg(void){
	configMsg.parseCharacter(ConfigMsg::OPEN_CHAR);
	while (_SERIAL_CONSOLE.available()){
		configMsg.parseCharacter(_SERIAL_CONSOLE.read());
		if (configMsg.isComplete()) break;
	}

	yield();

	_SERIAL_CONSOLE.println(configMsg.getMsgBuffer());

	configMsg.parseConfigMsg();

	if (configMsg.isValid()){
		_SERIAL_CONSOLE.println("---Config msg valid:");
		strcpy(ssid, configMsg.getSSID());
		strcpy(password, configMsg.getPW());
		_SERIAL_CONSOLE.println(ssid);
		_SERIAL_CONSOLE.println(password);
	}
	else {
		_SERIAL_CONSOLE.println("---Config msg invalid!");
	}

	configMsg.clear();
	return true;
}

bool readPM(void) {
	if (HKA5::MSG::LENGTH
			== _SERIAL_CONSOLE.readBytes(HKA5_msgBuffer, HKA5::MSG::LENGTH)) {
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

/*
 * CoAP callbacks #TODO sensor value access
 */

void COAP_callback_PM(coapPacket *packet, IPAddress ip, int port, int observer){
	_SERIAL_CONSOLE.println("PM get callback");
	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	_SERIAL_CONSOLE.println(p);

	char resp[50];
	sprintf(resp, "1=%d,2_5=%d,10=%d", 11, 22, 33);

	observer ? coap.sendResponse(resp) : coap.sendResponse(ip, port, resp);
}

void COAP_callback_pressure(coapPacket *packet, IPAddress ip, int port, int observer){
	_SERIAL_CONSOLE.println("Pressure get callback");
	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	_SERIAL_CONSOLE.println(p);

	char resp[50];
	sprintf(resp, "%d", 10000);

	observer ? coap.sendResponse(resp) : coap.sendResponse(ip, port, resp);
}

void COAP_callback_temperature(coapPacket *packet, IPAddress ip, int port, int observer){
	_SERIAL_CONSOLE.println("Temperature get callback");
	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	_SERIAL_CONSOLE.println(p);

	char resp[50];
	sprintf(resp, "%d", 25);

	observer ? coap.sendResponse(resp) : coap.sendResponse(ip, port, resp);
}

void COAP_callback_nodeInfo(coapPacket *packet, IPAddress ip, int port, int observer){
	_SERIAL_CONSOLE.println("Node info callback");
	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	_SERIAL_CONSOLE.println(p);

	observer ? coap.sendResponse(node_info_msg) : coap.sendResponse(ip, port, node_info_msg);
}
