const char* host     = "ESP32-OTA"; // Used for MDNS resolution
const char* ssid     = "ssid";
const char* password = "password";

#include <WebOTA.h>
WebServer OTAServer(8080);

long start       = 0;
long max_seconds = 30;
long i           = 2; // Start at 2
long found       = 0; // Number of primtes we've found
int LED_PIN      = 16;

void setup() {
	Serial.begin(115200);

	while (!Serial) { }

	pinMode(LED_PIN, OUTPUT);
	start = millis();

	init_wifi(ssid, password, host);
	init_web_ota(&OTAServer);
}

void loop() {
	digitalWrite(LED_PIN, HIGH);
	bool prime = is_prime(i); // Check if the number we're on is prime

	if (prime) {
		Serial.print(i);
		Serial.println(" is prime ");

		found++;
	}

	int running_seconds = (millis() - start) / 1000;

	if (max_seconds > 0 && (running_seconds >= max_seconds)) {
		Serial.print("Found ");
		Serial.print(found);
		Serial.print(" primes in ");
		Serial.print(max_seconds);
		Serial.println(" seconds");
		digitalWrite(LED_PIN, LOW);

		webota_delay(15 * 1000, &OTAServer);

		i     = 2;
		found = 0;
		start = millis();
	}

	i++;

	OTAServer.handleClient();
}

bool is_prime(long num) {
	// Only have to check for divisible for the sqrt(number)
	int upper = sqrt(num);

	// Check if the number is evenly divisible (start at 2 going up)
	for (long cnum = 2; cnum <= upper; cnum++) {
		long mod = num % cnum; // Remainder

		if (mod == 0) {
			return false;
		} // If the remainer is 0 it's evenly divisible
	}

	return true; // If you get this far it's prime
}



bool is_prime2(long number) {
	if (number < 2) return false;
	if (number == 2) return true;
	if (number % 2 == 0) return false;
	for(int i=3; (i*i)<=number; i+=2){
		if (number % i == 0 ) return false;
	}

	return true;
}
