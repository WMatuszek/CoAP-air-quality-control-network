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


static uint8_t HKA5_msgBuffer[HKA5::MSG::LENGTH];

coapServer coap;

/*
 * GLobal variables
 */

static NodeConfig NodeConfig;
static bool NodeConfigChanged = false;
static uint32_t nextSleep = 0;
static uint32_t nextServerReport = 0;

static HKA5::PMData_t Measured_PM = {0,0,0};
static float Measured_pressure = 0;
static float Measured_temperature = 0;

BMP280::BMP280Controller BMP280Ctrl(&Wire);
HKA5::HKA5Controller HKA5Ctrl(HKA5_POWER_CTRL_PIN);

ConfigMsg configMsg;

/*
 * CoAP resource access callbacks
 */
void COAP_callback_PM(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_pressure(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_temperature(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_battery(coapPacket *packet, IPAddress ip, int port, int observer); // Stub, unused
void COAP_callback_nodeInfo(coapPacket *packet, IPAddress ip, int port, int observer);
void COAP_callback_sleepCycle(coapPacket *packet, IPAddress ip, int port, int observer);

void setup() {
	yield();

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	delay(2000);
	yield();
	_SERIAL_CONSOLE.begin(HKA5::USART_BAUD_RATE);

	NodeConfig.setDefault();
	NodeConfig::LoadFromEEPROM(&NodeConfig);

	SensorsSetup();

	yield();

#ifdef WIFI_CONNECT_ON_STARTUP
	WiFiSetup();
	yield();
	CoAPSetup();
#endif

	CoAP_NodeReportToServer();
	nextServerReport = millis() + SEC_TO_MS(SERVER_REPORT_INTERVAL_SEC);

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
	WiFi.begin(NodeConfig.ssid, NodeConfig.password);
	uint32_t connectWaitStop = millis() + SEC_TO_MS(WIFI_CONNECT_WAIT_SEC);
	while (WiFi.status() != WL_CONNECTED ) {
		delay(500); yield();
		Serial.print(".");
		if (millis() > connectWaitStop) {
			_SERIAL_CONSOLE.println(" TIMEOUT! Will try again.");
			return;
		}
	}
	_SERIAL_CONSOLE.println(" CONNECTED! Local / gateway IP:");
	_SERIAL_CONSOLE.println(WiFi.localIP());
	_SERIAL_CONSOLE.println(WiFi.gatewayIP());
}

void CoAPSetup(){
	// CoAP config
	_SERIAL_CONSOLE.println("CoAP config");
	//coap.server(COAP_callback_response, "response");
	coap.server(COAP_callback_PM, "pm");
	coap.server(COAP_callback_pressure, "pressure");
	coap.server(COAP_callback_temperature, "temperature");
	coap.server(COAP_callback_nodeInfo, "sname");
	coap.server(COAP_callback_sleepCycle, "sleepcycle");
	coap.start();
}

void loop() {
	static uint32_t mainLoopCount = 0;
	const uint32_t REPORT_LOOP = 10;

	yield();
	coap.loop();
#ifndef NO_LED_BLINK
	digitalWrite(LED_BUILTIN, HIGH);
#endif
	delay(500);
	yield();
	coap.loop();
#ifndef NO_LED_BLINK
	digitalWrite(LED_BUILTIN, LOW);
#endif
	delay(500);
	yield();

	if (NodeConfig.sleepCycleActive && millis() > nextSleep) {
		_SERIAL_CONSOLE.println("Sleep\nzzZzzZz");
		sleepFor(NodeConfig.sleepTime_s);
		nextSleep = millis() + SEC_TO_MS(NodeConfig.measureTime_s);
		_SERIAL_CONSOLE.println("Wake");
	}

	if (WiFi.isConnected() == false){
		_SERIAL_CONSOLE.println("WIFI dc'd, try reconnect");
		WiFi.reconnect();
	}

	if (millis() > nextServerReport) {
		CoAP_NodeReportToServer();
		nextServerReport = millis() + SEC_TO_MS(SERVER_REPORT_INTERVAL_SEC);
		_SERIAL_CONSOLE.println("Report to server");
	}

	if (NodeConfigChanged) {
		NodeConfig::SaveToEEPROM(&NodeConfig);
		NodeConfigChanged = false;
	}

	USARTSerialInputCheck();										// Read either HK-A5 output or config msg

	if (BMP280Ctrl.isModeContinous(SType_t::TEMP))					// BMP280 read if in continous mode
		Measured_temperature = BMP280Ctrl.measureTemperature();		// Read temperature
	if (BMP280Ctrl.isModeContinous(SType_t::PRESS))
		Measured_pressure = BMP280Ctrl.measurePressure();			// Read pressure


	++mainLoopCount;
}

void sleepFor(uint16_t sleep_s) {
	// #TODO this is a placeholder
	// ESP.deepSleep(SEC_TO_US(sleep_s)); <-- does not wake up
}

/*
 *  -----------------------------------------------------------------------
 */

bool USARTSerialInputCheck(void){
	while (_SERIAL_CONSOLE.available()){
		uint8_t token = _SERIAL_CONSOLE.read();
		if (token == HKA5::MSG::OPEN_TOKEN)
			return ReadPM();
		if (token == ConfigMsg::OPEN_CHAR) {
			if (ReadNodeConfigMsg()) {
				WiFi.disconnect();
				WiFiSetup();
			}
		}
	}
	return false;
}

bool ReadNodeConfigMsg(void){
	configMsg.clear();
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
		strcpy(NodeConfig.nodeName, configMsg.getNodeName());
		strcpy(NodeConfig.ssid, configMsg.getSSID());
		strcpy(NodeConfig.password, configMsg.getPW());
		NodeConfigChanged = true;
		_SERIAL_CONSOLE.println(NodeConfig.nodeName);
		_SERIAL_CONSOLE.println(NodeConfig.ssid);
		_SERIAL_CONSOLE.println(NodeConfig.password);
		return true;
	}
	_SERIAL_CONSOLE.println("---Config msg invalid!");
	return false;
}

bool ReadPM(void) {
	if (HKA5::MSG::LENGTH
			== _SERIAL_CONSOLE.readBytes(HKA5_msgBuffer, HKA5::MSG::LENGTH)) {
		Measured_PM = HKA5Ctrl.getPM();
		return true;
	}
	return false;
}

void PrintPM(void){
	_SERIAL_CONSOLE.println("PM values [ug/m3]:");
	_SERIAL_CONSOLE.print("PM 1 = ");
	_SERIAL_CONSOLE.println(Measured_PM.PM_1);
	_SERIAL_CONSOLE.print("PM 2.5 = ");
	_SERIAL_CONSOLE.println(Measured_PM.PM_2_5);
	_SERIAL_CONSOLE.print("PM 10 = ");
	_SERIAL_CONSOLE.println(Measured_PM.PM_10);
}

uint32_t GetBatteryStatus(void) {
	// #TODO dummy
	return 100;
}

uint16_t CoAP_Ping(IPAddress ip, uint16_t port) {
	coapPacket packet;

	// Make packet
	packet.version = 1;
	packet.type = COAP_TYPE::COAP_CON;
	packet.code = COAP_METHOD::COAP_EMPTY;
	packet.token = NULL;
	packet.tokenlen = 0;
	packet.payload = NULL;
	packet.payloadlen = 0;
	packet.optionnum = 0;
	packet.messageid = rand();

	return coap.sendPacket(&packet, ip, port);
}

/*
 * Get server report resource
 * Send GET request to SERVER_REPORT_URI, at gateway IP
 */
uint16_t CoAP_NodeReportToServer() {
	IPAddress servIP = WiFi.gatewayIP();
	coapPacket packet;

	// Make packet
	packet.version = 1;
	packet.type = COAP_TYPE::COAP_CON;
	packet.code = COAP_METHOD::COAP_EMPTY;
	packet.token = NULL;
	packet.tokenlen = 0;
	packet.payload = NULL;
	packet.payloadlen = 0;
	packet.optionnum = 0;
	packet.messageid = rand();

	return coap.sendPacket(&packet, servIP, COAP_DEFAULT_PORT);
}

/*
 * CoAP callbacks
 */

void COAP_callback_PM(coapPacket *packet, IPAddress ip, int port, int observer){
	const size_t respBufferSize = 50;
	static bool flag = true;
	static char buff1[respBufferSize], buff2[respBufferSize];

	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	if (!observer) {
		_SERIAL_CONSOLE.print("PM get callback, payload: ");
		_SERIAL_CONSOLE.println(p);
	}

	char *resp = buff1;
	if (flag)
		resp = buff2;
	flag = !flag;

	if (packet->code_() != COAP_METHOD::COAP_GET && observer) {
		return; // POST + OBSERVE bug in library
	}
	if (packet->code_() == COAP_METHOD::COAP_GET) {
		using namespace ArduinoJson;
		const size_t bufferSize = JSON_OBJECT_SIZE(3);
		DynamicJsonBuffer jsonBuffer(bufferSize);

		JsonObject& root = jsonBuffer.createObject();
		root["pm1"] = Measured_PM.PM_1;
		root["pm2_5"] = Measured_PM.PM_2_5;
		root["pm10"] = Measured_PM.PM_10;

		root.printTo(resp, respBufferSize);
	}
	if (packet->code_() == COAP_METHOD::COAP_POST) {
		using namespace ArduinoJson;
		const size_t bufferSize = JSON_OBJECT_SIZE(2) + 20;
		DynamicJsonBuffer jsonBuffer(bufferSize);

		JsonObject& root = jsonBuffer.parseObject(p);

		if (root.success()) {
			uint8_t state = root["on"];
			uint8_t mode = root["mode"];
			HKA5Ctrl.setState(SType_t::AIRQ, (SState_t)state);
			HKA5Ctrl.setMeasureMode(SType_t::AIRQ, (SMeasureMode_t)mode);
			_SERIAL_CONSOLE.println("PM sensor settings updated");
			strcpy(resp, RESPONSE_OK);
		}
		else {
			strcpy(resp, RESPONSE_FAIL);
		}
	}

	observer ? coap.sendResponse(resp) : coap.sendResponse(ip, port, resp);
}

void COAP_callback_pressure(coapPacket *packet, IPAddress ip, int port, int observer){
	_SERIAL_CONSOLE.print("Pressure get callback, payload: ");
	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	_SERIAL_CONSOLE.println(p);

	char resp[30];
	if (packet->code_() == COAP_METHOD::COAP_GET) {
		if (!BMP280Ctrl.isSensorOn(SType_t::PRESS))
			strcpy(resp, "OFF");
		else {
			if (BMP280Ctrl.isModeContinous(SType_t::PRESS))
				dtostrf(Measured_pressure, 5, 2, resp); // Use existing value
			else
				dtostrf(BMP280Ctrl.measurePressure(), 5, 2, resp); // Measure on demand
		}
	}
	if (packet->code_() == COAP_METHOD::COAP_POST) {
		using namespace ArduinoJson;
		const size_t bufferSize = JSON_OBJECT_SIZE(2) + 20;
		DynamicJsonBuffer jsonBuffer(bufferSize);

		JsonObject& root = jsonBuffer.parseObject(p);

		if (root.success()) {
			uint8_t state = root["on"];
			uint8_t mode = root["mode"];
			BMP280Ctrl.setState(SType_t::PRESS, (SState_t)state);
			BMP280Ctrl.setMeasureMode(SType_t::PRESS, (SMeasureMode_t)mode);
			_SERIAL_CONSOLE.println("Press sensor settings updated");
			strcpy(resp, RESPONSE_OK);
		}
		else {
			_SERIAL_CONSOLE.println("Press sensor settings update fail");
			strcpy(resp, RESPONSE_FAIL);
		}
	}

	observer ? coap.sendResponse(resp) : coap.sendResponse(ip, port, resp);
}

void COAP_callback_temperature(coapPacket *packet, IPAddress ip, int port, int observer){
	_SERIAL_CONSOLE.print("Temperature get callback, payload: ");
	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	_SERIAL_CONSOLE.println(p);

	char resp[30];
	if (packet->code_() == COAP_METHOD::COAP_GET) {
		if (!BMP280Ctrl.isSensorOn(SType_t::TEMP))
			strcpy(resp, "OFF");
		else {
			if (BMP280Ctrl.isModeContinous(SType_t::TEMP))
				dtostrf(Measured_temperature, 3, 2, resp); // Use existing value
			else
				dtostrf(BMP280Ctrl.measureTemperature(), 3, 2, resp); // Measure on demand
		}
	}
	if (packet->code_() == COAP_METHOD::COAP_POST) {
		using namespace ArduinoJson;
		const size_t bufferSize = JSON_OBJECT_SIZE(2) + 20;
		DynamicJsonBuffer jsonBuffer(bufferSize);

		JsonObject& root = jsonBuffer.parseObject(p);

		if (root.success()) {
			uint8_t state = root["on"];
			uint8_t mode = root["mode"];
			BMP280Ctrl.setState(SType_t::TEMP, (SState_t)state);
			BMP280Ctrl.setMeasureMode(SType_t::TEMP, (SMeasureMode_t)mode);
			_SERIAL_CONSOLE.println("Temp sensor settings updated");
			strcpy(resp, RESPONSE_OK);
		}
		else {
			_SERIAL_CONSOLE.println("Temp sensor settings update fail");
			strcpy(resp, RESPONSE_FAIL);
		}
	}

	observer ? coap.sendResponse(resp) : coap.sendResponse(ip, port, resp);
}

void COAP_callback_battery(coapPacket *packet, IPAddress ip, int port, int observer){
	_SERIAL_CONSOLE.print("Battery get callback, payload: ");
	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	_SERIAL_CONSOLE.println(p);

	char resp[10];
	sprintf(resp, "%d", GetBatteryStatus());

	observer ? coap.sendResponse(resp) : coap.sendResponse(ip, port, resp);
}

void COAP_callback_nodeInfo(coapPacket *packet, IPAddress ip, int port, int observer){
	_SERIAL_CONSOLE.print("Node info callback, payload: ");
	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	_SERIAL_CONSOLE.println(p);

	char resp[50];
	if (packet->code_() == COAP_METHOD::COAP_GET) {
		sprintf(resp, "Node:%s", NodeConfig.nodeName);
	}
	if (packet->code_() == COAP_METHOD::COAP_POST) {
		strcpy(NodeConfig.nodeName, p);
		NodeConfigChanged = true;
		_SERIAL_CONSOLE.println("Station name updated");
		strcpy(resp, RESPONSE_OK);
	}

	observer ? coap.sendResponse(resp) : coap.sendResponse(ip, port, resp);
}

void COAP_callback_sleepCycle(coapPacket *packet, IPAddress ip, int port, int observer){
	_SERIAL_CONSOLE.print("Node info callback, payload: ");
	char p[packet->payloadlen + 1];
	memcpy(p, packet->payload, packet->payloadlen);
	p[packet->payloadlen] = '\0';
	_SERIAL_CONSOLE.println(p);

	const size_t respBufferSize = 50;
	char resp[respBufferSize];

	if (packet->code_() == COAP_METHOD::COAP_GET) {
		using namespace ArduinoJson;
		const size_t bufferSize = JSON_OBJECT_SIZE(3);
		DynamicJsonBuffer jsonBuffer(bufferSize);

		JsonObject& root = jsonBuffer.createObject();
		root["on"] = NodeConfig.sleepCycleActive;
		root["sleep"] = NodeConfig.sleepTime_s;
		root["measure"] = NodeConfig.measureTime_s;
		root.printTo(resp, respBufferSize);
	}

	if (packet->code_() == COAP_METHOD::COAP_POST) {
		using namespace ArduinoJson;
		const size_t bufferSize = JSON_OBJECT_SIZE(3) + 40;
		DynamicJsonBuffer jsonBuffer(bufferSize);

		JsonObject& root = jsonBuffer.parseObject(p);

		if (root.success()) {
			NodeConfig.sleepCycleActive = root["on"];
			NodeConfig.sleepTime_s = root["sleep"];
			NodeConfig.measureTime_s = root["measure"];
			NodeConfigChanged = true;
			_SERIAL_CONSOLE.println("Sleep cycle updated");
			strcpy(resp, RESPONSE_OK);
		}
		else {
			_SERIAL_CONSOLE.println("Sleep cycle update fail");
			strcpy(resp, RESPONSE_FAIL);
		}
	}

	observer ? coap.sendResponse(resp) : coap.sendResponse(ip, port, resp);
}
