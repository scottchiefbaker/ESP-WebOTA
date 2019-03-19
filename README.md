# ESP WebOTA

Easily add web based [OTA](https://en.wikipedia.org/wiki/Over-the-air_programming) updates to your ESP32/ESP8266 projects.

## Installation

Clone this repo to your Arduino libraries directory. On Linux this is `~/Arduino/libraries/`

## Usage

Include the WebOTA library

    #include <WebOTA.h>

Optionally initialize the WebOTA library if you want to change the defaults. This is done at the end of your `setup()` function:

    void setup() {
        // Other init code here (WiFi, etc)

        // To use a specific port and path uncomment this line
        // Defaults are 8080 and "/webota"
        // webota.init(8888, "/update");
    }

Listen for update requests at the end of your `loop()` function:

    void loop() {
        // Other loop code here

        webota.handle();
    }

**Note:** If you have long `delay()` commands in your `loop()` WebOTA may not be responsive. We have provided `webota.delay()` as a drop-in replacement, which is more WebOTA friendly.

## Upload a sketch

You will need to create a binary sketch image to upload. This is done in the Arduino IDE by going to the `Sketch` menu and selecting `Export compiled Binary`.

Navigate to your ESP in a web browser to upload your binary image. Typical URLs are: http://esp-ota.local:8080/webota.

You can also use Curl if you want to script your uploads from the CLI

    curl -F "file=@MyImage.bin" http://esp-ota.local:8080/webota

## Based on

Borrowed from [randomnerdtutorials.com](https://randomnerdtutorials.com/esp32-over-the-air-ota-programming/) and improved upon.
