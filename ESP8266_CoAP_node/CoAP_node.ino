#include "CoAP_node.h"

#include "Arduino.h"
#include "Wire.h"
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>

#include <coap_server.h>
#include <coap_client.h>

#include "QueueArray.h"

#include "HKA5Controller.h"
#include "BMP280Controller.h"
#include "ConfigMsg.h"

static char ssid[WIFI_CRED_BUFFER_SIZE]			= "OpiAP";
static char password[WIFI_CRED_BUFFER_SIZE] 	= "herpderp";

static char node_info_msg[50] 					= "Node info here";

static uint8_t HKA5_msgBuffer[HKA5::MSG::LENGTH];

coapServer coap;

/*
 * GLobal variables
 */
static HKA5::PMData_t Measure_PM = {0,0,0};
static float Measure_press = 0;
static float Measure_temp = 0;

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
void COAP_callback_response(coapPacket *packet, IPAddress ip, int port, int observer) {} // { _SERIAL_CONSOLE.println("RESP"); }

void setup() {
	yield();

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	delay(2000);

	yield();

	_SERIAL_CONSOLE.begin(HKA5::USART_BAUD_RATE);

	SensorsSetup();

	yield();

#ifdef WIFI_CONNECT_ON_STARTUP
	WiFiSetup();
	yield();
	CoAPSetup();
#endif

	NodeReportToServer();

	_SERIAL_CONSOLE.println("--- Config done");

}

void SensorsSetup() {
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

void WiFiSetup(){
	// WIFI config
	_SERIAL_CONSOLE.println("\r\nWIFI config");
//	WiFi.setAutoConnect(false);
	WiFi.persistent(true);
	WiFi.mode(WiFiMode_t::WIFI_STA);
	WiFi.setSleepMode(WiFiSleepType_t::WIFI_NONE_SLEEP);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500); yield();
		Serial.print(".");
	}
	_SERIAL_CONSOLE.println(" CONNECTED!");
	_SERIAL_CONSOLE.println(WiFi.localIP());
	_SERIAL_CONSOLE.println(WiFi.gatewayIP());
}

void CoAPSetup(){
	// CoAP config
	_SERIAL_CONSOLE.println("CoAP config");
	coap.server(COAP_callback_response, "response");
	coap.server(COAP_callback_PM, "pm");
	coap.server(COAP_callback_pressure, "pressure");
	coap.server(COAP_callback_temperature, "temperature");
	coap.server(COAP_callback_nodeInfo, "info");
	coap.start();
}

void loop() {
	static uint32_t mainLoopCount = 0;
	const uint32_t REPORT_LOOP = 10;

	yield();
	coap.loop();

	digitalWrite(LED_BUILTIN, HIGH);
	delay(1000);

	yield();
	coap.loop();

	digitalWrite(LED_BUILTIN, LOW);
	delay(1000);

	USARTSerialInputCheck();

	if (WiFi.isConnected() == false){
		_SERIAL_CONSOLE.println("WIFI disconnected, attempting to reconnect");
		WiFi.reconnect();
	}
	if (!Ping.ping(WiFi.gatewayIP(), 1)){
		_SERIAL_CONSOLE.println("Ping fail!");
	}

	if (mainLoopCount % REPORT_LOOP == 0) {
		NodeReportToServer();
		_SERIAL_CONSOLE.println("Report to server");
	}

	Measure_temp = BMP280Ctrl.readTemperature();
	Measure_press = BMP280Ctrl.readPressure();

	++mainLoopCount;
}

/*
 *  -----------------------------------------------------------------------
 */

bool USARTSerialInputCheck(void){
	while (_SERIAL_CONSOLE.available()){
		uint8_t token = _SERIAL_CONSOLE.read();
		if (token == HKA5::MSG::OPEN_TOKEN)
			ReadPM();
		if (token == ConfigMsg::OPEN_CHAR)
			ReadNodeConfigMsg();
	}
	return false;
}

bool ReadNodeConfigMsg(void){
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

bool ReadPM(void) {
	if (HKA5::MSG::LENGTH
			== _SERIAL_CONSOLE.readBytes(HKA5_msgBuffer, HKA5::MSG::LENGTH)) {
		Measure_PM = HKA5Ctrl.getPM();
		return true;
	}
	return false;
}

void PrintPM(void){
	_SERIAL_CONSOLE.println("PM values [ug/m3]:");
	_SERIAL_CONSOLE.print("PM 1 = ");
	_SERIAL_CONSOLE.println(Measure_PM.PM_1);
	_SERIAL_CONSOLE.print("PM 2.5 = ");
	_SERIAL_CONSOLE.println(Measure_PM.PM_2_5);
	_SERIAL_CONSOLE.print("PM 10 = ");
	_SERIAL_CONSOLE.println(Measure_PM.PM_10);
}

/*
 * Report to server
 * Send GET request to SERVER_REPORT_URI, at gateway IP
 */
uint16_t NodeReportToServer() {
	IPAddress servIP = WiFi.gatewayIP();
	coapPacket packet;

	// Make packet
	packet.type = COAP_TYPE::COAP_CON;
	packet.code = COAP_METHOD::COAP_GET;
	packet.token = NULL;
	packet.tokenlen = 0;
	packet.payload = NULL;
	packet.payloadlen = 0;
	packet.optionnum = 0;
	packet.messageid = rand();

	packet.options[packet.optionnum].buffer = (uint8_t *)SERVER_REPORT_URI;
	packet.options[packet.optionnum].length = strlen(SERVER_REPORT_URI);
	packet.options[packet.optionnum].number = COAP_OPTION_NUMBER::COAP_URI_PATH;
	packet.optionnum++;

	return coap.sendPacket(&packet, servIP, COAP_DEFAULT_PORT);
}

/*
 * CoAP callbacks #TODO sensor value access
 */

void COAP_callback_PM(coapPacket *packet, IPAddress ip, int port, int observer){
	_SERIAL_CONSOLE.print("PM get callback: ");
	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	_SERIAL_CONSOLE.println(p);

	char resp[50];
	sprintf(resp, "1=%d,2_5=%d,10=%d", Measure_PM.PM_1, Measure_PM.PM_2_5, Measure_PM.PM_10);

	observer ? coap.sendResponse(resp) : coap.sendResponse(ip, port, resp);
}

void COAP_callback_pressure(coapPacket *packet, IPAddress ip, int port, int observer){
	_SERIAL_CONSOLE.print("Pressure get callback: ");
	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	_SERIAL_CONSOLE.println(p);

	char resp[10];
	dtostrf(Measure_press, 5, 2, resp);
	resp[9] = '\0';

	observer ? coap.sendResponse(resp) : coap.sendResponse(ip, port, resp);
}

void COAP_callback_temperature(coapPacket *packet, IPAddress ip, int port, int observer){
	_SERIAL_CONSOLE.print("Temperature get callback: ");
	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	_SERIAL_CONSOLE.println(p);

	char resp[10];
	dtostrf(Measure_temp, 3, 2, resp);
	resp[9] = '\0';

	observer ? coap.sendResponse(resp) : coap.sendResponse(ip, port, resp);
}

void COAP_callback_nodeInfo(coapPacket *packet, IPAddress ip, int port, int observer){
	_SERIAL_CONSOLE.print("Node info callback: ");
	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	_SERIAL_CONSOLE.println(p);

	observer ? coap.sendResponse(node_info_msg) : coap.sendResponse(ip, port, node_info_msg);
}
