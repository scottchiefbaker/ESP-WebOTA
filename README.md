# ESP WebOTA

Easily add web based [OTA](https://en.wikipedia.org/wiki/Over-the-air_programming) updates to your ESP32 projects.

## Installation

Clone this repo to your Arduino libraries directory. On Linux this is `~/Arduino/libraries/`

## Usage

Create a global variable for the Web Server:

    #include <WebOTA.h>
    WebServer OTAServer(8080);

Initialize the WebOTA library at the end of your `setup()` function:

    void setup() {
        // Other init code here

        init_web_ota(&OTAServer);
    }

Listen for update requests at the end of your `loop()` function:

    void loop() {
        // Other loop code here

        OTAServer.handleClient();
    }

## Perform an update

Navigate to your ESP32 in a web browser, typical URLs are: http://esp32-ota.local:8080/webota. To create a binary image you will need the Arduino IDE. A binary image can be created by going the `Sketch` menu and selecting `Export compiled Binary`.

## Based on

Borrowed from [randomnerdtutorials.com](https://randomnerdtutorials.com/esp32-over-the-air-ota-programming/) and improved upon.
