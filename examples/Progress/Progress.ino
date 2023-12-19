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

    webota
    .onStart([]() {
      Serial.println("Start updating");
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    // .onProgress([](unsigned int progress, unsigned int total) {
    //   Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    // })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

	// Defaults to 8080 and "/webota"
	//webota.init(80, "/update");
}

// the loop function runs over and over again forever
void loop() {
	int md = 1000;

	digitalWrite(LED_PIN, HIGH);
	webota.delay(md);
	digitalWrite(LED_PIN, LOW);
	webota.delay(md);

	webota.handle();
}
