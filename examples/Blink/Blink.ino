const char* host     = "ESP32-OTA"; // Used for MDNS resolution
const char* ssid     = "ssid";
const char* password = "password";

#include <WebOTA.h>
WebServer OTAServer(8080);

#define LED_PIN 2

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);

	// initialize digital pin LED_PIN as an output.
	pinMode(LED_PIN, OUTPUT);

	init_wifi(ssid, password, host);
	init_web_ota(&OTAServer);
}

// the loop function runs over and over again forever
void loop() {
	int md = 1000;

	digitalWrite(LED_PIN, HIGH);
	webota_delay(md);
	digitalWrite(LED_PIN, LOW);
	webota_delay(md);

	OTAServer.handleClient();
}
