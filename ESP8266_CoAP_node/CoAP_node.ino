#include "Arduino.h"

#include "WiFi.h"
#include "WiFiUdp.h"

#include "coap.h"

// UDP and CoAP class
WiFiUDP udp;
Coap coap(udp);

void setup() {
	pinMode(LED_BUILTIN, OUTPUT);
	Serial.begin(115200);
}

void loop() {
	digitalWrite(LED_BUILTIN, HIGH);
	delay(500);
	digitalWrite(LED_BUILTIN, LOW);
	delay(500);

}
