# ESP WebOTA

Easily add web based [OTA](https://en.wikipedia.org/wiki/Over-the-air_programming) updates to your ESP32 projects.

## Installation

Clone this repo to your Arduino libraries directory. On Linux this is `~/Arduino/libraries/`

## Usage

Include the WebOTA library

    #include <WebOTA.h>
    WebServer OTAServer(8080);

Optionally initialize the WebOTA library if you want to change the defaults . This is done at the end of your `setup()` function:

    void setup() {
        // Other init code here (WiFi, etc)

		// The defaults are 8080 and "/webota"
		// if you omit init_web_ota() it will use the defaults
        init_web_ota(8888, "/update");
    }

Listen for update requests at the end of your `loop()` function:

    void loop() {
        // Other loop code here

		handle_webota();
    }

**Note:** If you have long `delay()` commands in your `loop()` WebOTA may not be responsive. We have provided `webota_delay()` as a drop-in replacement, which is more WebOTA friendly.

## Upload a sketch

You will need to create a binary sketch image to upload. This is done in the Arduino IDE by going to the `Sketch` menu and selecting `Export compiled Binary`.

Navigate to your ESP32 in a web browser to upload your binary image. Typical URLs are: http://esp32-ota.local:8080/webota.

## Based on

Borrowed from [randomnerdtutorials.com](https://randomnerdtutorials.com/esp32-over-the-air-ota-programming/) and improved upon.
