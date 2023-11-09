const char* host     = "ESP-OTA"; // Used for MDNS resolution
const char* ssid     = "ssid";
const char* password = "password";

#include <WebOTA.h>

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);

	init_wifi(ssid, password, host);

	// If you call useAuth() in your setup function WebOTA will use
	// HTTP digest authentication to request credentials from the user
	// before allowing uploads
	webota.useAuth("username", "password");
}

// the loop function runs over and over again forever
void loop() {
	// Do other stuff

	webota.handle();
}
