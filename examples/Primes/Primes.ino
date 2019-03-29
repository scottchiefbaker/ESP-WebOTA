const char* host     = "ESP-OTA"; // Used for MDNS resolution
const char* ssid     = "ssid";
const char* password = "password";

#include <WebOTA.h>

long start       = 0;
long max_seconds = 30;
long i           = 2; // Start at 2
long found       = 0; // Number of primtes we've found
int LED_PIN      = 16;

bool is_prime(long num) {
	// Only have to check for divisible for the sqrt(number)
	int upper = sqrt(num);

	// Check if the number is evenly divisible (start at 2 going up)
	for (long cnum = 2; cnum <= upper; cnum++) {
		long mod = num % cnum; // Remainder

		// If the remainer is 0 it's evenly divisible
		if (mod == 0) {
			return false;
		}
	}

	// If you get this far it's prime
	return true;
}

////////////////////////////////////////////////////

void setup() {
	Serial.begin(115200);

	pinMode(LED_PIN, OUTPUT);
	start = millis();

	init_wifi(ssid, password, host);

	// Defaults to 8080 and "/webota"
	//webota.init(80, "/update");
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

		webota.delay(15000);

		i     = 2;
		found = 0;
		start = millis();
	}

	i++;

	webota.handle();
}
