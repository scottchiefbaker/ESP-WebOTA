const char* host     = "ESP-OTA"; // Used for MDNS resolution
const char* ssid     = "ssid";
const char* password = "password";

#include <WebOTA.h>

#define LED_PIN 2

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);

	// initialize digital pin LED_PIN as an output.
	pinMode(LED_PIN, OUTPUT);

	init_wifi(ssid, password, host);

	// Defaults to 8080 and "/webota"
	//init_webota(80, "/update");
}

// the loop function runs over and over again forever
void loop() {
	int md = 1000;

	digitalWrite(LED_PIN, HIGH);
	webota_delay(md);
	digitalWrite(LED_PIN, LOW);
	webota_delay(md);

	handle_webota();
}
