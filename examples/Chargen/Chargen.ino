const char* host     = "ESP-OTA"; // Used for MDNS resolution
const char* ssid     = "ssid";
const char* password = "password";

#include <WebOTA.h>

void setup() {
	Serial.begin(115200);

	init_wifi(ssid, password, host);

	// Defaults to 8080 and "/webota"
	//webota.init(80, "/update");
}

int offset = 0;
void loop() {
	// Whatever string we want to spit out
	const char* str = "abcdefghijklmnopqrtuvwxyz01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()";
	int len         = strlen(str);

	for (int i = 0; i < len; i++) {
		int char_num = i + offset;
		char_num     = char_num % len; // Roll around the end if we go to far

		char c = str[char_num];

		Serial.print(c);
	}

	Serial.print("\r\n");
	offset++;

	if (offset >= len) { offset = 0; }

    webota.handle();
}
