// Arduino build process info: https://github.com/arduino/Arduino/wiki/Build-Process

#define WEBOTA_VERSION "0.1.6"

#include "WebOTA.h"
#include <Arduino.h>
#include <WiFiClient.h>

#ifdef ESP32
#include <ESPmDNS.h>
#include <Update.h>
#include <WiFi.h>
#include <AsyncTCP.h>

#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>

#endif

AsyncWebServer OTAServer(8080);
WebOTA webota;

////////////////////////////////////////////////////////////////////////////

int WebOTA::init(const unsigned int port, const char *path) {
	this->port = port;
	this->path = path;

	// Only run this once
	if (this->init_has_run) {
		return 0;
	}

	add_http_routes(&OTAServer, path);

	OTAServer.begin();

	Serial.printf("WebOTA url   : http://%s.local:%d%s\r\n\r\n", this->mdns.c_str(), port, path);

	// Store that init has already run
	this->init_has_run = true;

	return 1;
}

// One param
int WebOTA::init(const unsigned int port) {
	return WebOTA::init(port, "/webota");
}

// No params
int WebOTA::init() {
	return WebOTA::init(8080, "/webota");
}

int WebOTA::handle() {
	// If we haven't run the init yet run it
	if (!this->init_has_run) {
		WebOTA::init();
	}

#ifdef ESP8266
	MDNS.update();
#endif

	return 1;
}

long WebOTA::max_sketch_size() {
	long ret = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;

	return ret;
}

// R Macro string literal https://en.cppreference.com/w/cpp/language/string_literal
const char INDEX_HTML[] PROGMEM = R"!^!(
<!doctype html>
<html lang="en">
<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1">
	<title>WebOTA</title>

	<script src="/main.js"></script>
	<link rel="stylesheet" href="/main.css">
</head>
<body>
	<h1>WebOTA version %s</h1>

	<form method="POST" action="#" enctype="multipart/form-data" id="upload_form">
		<div><input class="" type="file" name="update" id="file"></div>

		<div><input class="btn" type="submit" value="Upload"></div>
	</form>

	<p>
		<b>Board:</b> %s<br />
	</p>

	<div>
		<div id="prg" class="prog_bar" style=""></div>
	</div>
  </body>
</html>
)!^!";

const char MAIN_CSS[] PROGMEM = R"!^!(
body {
	margin: 2em;
	font-family: sans-serif;
}

input[type=file]::file-selector-button, .btn {
	margin-right: 20px;
	border: 1px solid gray;
	background-image: linear-gradient(to bottom,#59ef59 0, #55982f 100%);
	padding: 10px 20px;
	border-radius: 4px;
	color: #fff;
	cursor: pointer;
	transition: background .2s ease-in-out;
	width: 10em;
}

.btn {
	margin-top: 12px;
	background-image: linear-gradient(to bottom,#78addb 0,#2d6ca2 100%);
}

.prog_bar {
	margin: 12px 0;
	text-shadow: 2px 2px 3px black;
	padding: 5px 0;
	display: none;
	border: 1px solid #7c7c7c;
	background-image: linear-gradient(to right,#d2d2d2 0,#2d2d2d 100%);
	line-height: 1.3em;
	border-radius: 4px;
	text-align: center;
	color: white;
	font-size: 250%;
}
)!^!";

const char MAIN_JS[] PROGMEM = R"!^!(
var domReady = function(callback) {
	document.readyState === "interactive" || document.readyState === "complete" ? callback() : document.addEventListener("DOMContentLoaded", callback);
};

domReady(function() {
	var myform = document.getElementById('upload_form');
	var filez  = document.getElementById('file');

	myform.onsubmit = function(event) {
		event.preventDefault();

		var formData = new FormData();
		var file     = filez.files[0];

		if (!file) { return false; }

		formData.append("files", file, file.name);

		var xhr = new XMLHttpRequest();
		xhr.upload.addEventListener("progress", function(evt) {
			if (evt.lengthComputable) {
				var per = Math.round((evt.loaded / evt.total) * 100);
				var prg = document.getElementById('prg');

				var str = per + "%";
				prg.style.width   = str;

				prg.innerHTML = str;

				prg.style.display = "block";
			}
		}, false);
		xhr.open('POST', location.href, true);

		// Set up a handler for when the request finishes.
		xhr.onload = function () {
			if (xhr.status === 200) {
				//alert('Success');
			} else {
				//alert('An error occurred!');
			}
		};

		xhr.send(formData);
   }
});
)!^!";

void handle404(AsyncWebServerRequest *request){
	//Handle Unknown Request
	request->send(404, "text/html", "File not found");
}

// Handling uploading firmware file
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool is_final) {
	uint32_t totalSize = index + len;

	// Starting the upload
	if (!index) {
		Serial.printf("Firmware update initiated: %s\r\n", filename.c_str());

		uint32_t maxSketchSpace = webota.max_sketch_size();

		if (!Update.begin(maxSketchSpace)) { //start with max available size
			Update.printError(Serial);
		}

		//Serial.printf("UploadStart: %s\n", filename.c_str());
	}

	// Chunk in the middle
	if (len) {
		//Serial.printf("Writing %d / %d: %s\r\n", len, totalSize, filename.c_str());

		// flashing firmware to ESP
		if (Update.write(data, len) != len) {
			Serial.printf("Error writing chunk\r\n");
			Update.printError(Serial);
		}

		// Store the next milestone to output
		uint16_t chunk_size  = 51200;
		static uint32_t next = 51200;

		// Check if we need to output a milestone (100k 200k 300k)
		if (totalSize >= next) {
			Serial.printf("%dk ", next / 1024);
			next += chunk_size;
		}
	}

	// Upload complete
	if (is_final) {
		if (!Update.hasError()) {
			Serial.printf("\r\nFirmware update successful: %u bytes\r\nRebooting...\r\n", totalSize);

			request->send(200, "text/plain", "Update: OK!\n");
			delay(500);
			ESP.restart();
		} else {
			Serial.printf("\r\nSome error: %u bytes\r\nRebooting...\r\n", totalSize);
			request->send(200, "text/plain", "Update: ERROR!\n");

			Update.printError(Serial);
		}
	}
}

String WebOTA::get_board_type() {

#if defined(ESP8266)
	String BOARD_NAME = "ESP8266";
#elif defined(ESP32S2)
	String BOARD_NAME = "ESP32-S2";
#elif defined(ESP32)
	String BOARD_NAME = "ESP32";
#else
	String BOARD_NAME = "Unknown";
#endif

	return BOARD_NAME;
}

int WebOTA::add_http_routes(AsyncWebServer *server, const char *path) {

	// FILE: main.js
	OTAServer.on("/main.js", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send_P(200, "application/javascript", MAIN_JS);
	});

	// FILE: main.css
	OTAServer.on("/main.css", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send_P(200, "text/css", MAIN_CSS);
	});

	// FILE: favicon.ico
	OTAServer.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(200, "image/vnd.microsoft.icon", "");
	});

	// Handling uploading firmware file
	OTAServer.on(path, HTTP_POST, [this](AsyncWebServerRequest *request) {
		request->send(200);
	}, handleUpload);

	// Upload firmware page
	OTAServer.on(path, HTTP_GET, [this](AsyncWebServerRequest *request) {
		String html = "";

		if (this->custom_html != NULL) {
			html = this->custom_html;
		} else {
			String BOARD_TYPE = this->get_board_type();
			//size_t x = this->max_sketch_size();

			char buf[1024] = "";
			snprintf_P(buf, sizeof(buf), INDEX_HTML, WEBOTA_VERSION, BOARD_TYPE.c_str());

			html = buf;
		}

		request->send_P(200, "text/html", html.c_str());
	});

	/*
	OTAServer.on(path, HTTP_POST, [this](AsyncWebServerRequest *request) {
		request->send(200, "text/plain", (Update.hasError()) ? "Update: fail\n" : "Update: OK!\n");
		delay(500);
		ESP.restart();
	}, [this](AsyncWebServerRequest *request) {
		HTTPUpload& upload = request->upload();

		if (upload.status == UPLOAD_FILE_START) {
			Serial.printf("Firmware update initiated: %s\r\n", upload.filename.c_str());

			//uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;

			if (!Update.begin(maxSketchSpace)) { //start with max available size
				Update.printError(Serial);
			}
		} else if (upload.status == UPLOAD_FILE_WRITE) {
			// flashing firmware to ESP
			if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
				Update.printError(Serial);
			}

			// Store the next milestone to output
			uint16_t chunk_size  = 51200;
			static uint32_t next = 51200;

			// Check if we need to output a milestone (100k 200k 300k)
			if (upload.totalSize >= next) {
				Serial.printf("%dk ", next / 1024);
				next += chunk_size;
			}
		} else if (upload.status == UPLOAD_FILE_END) {
			if (Update.end(true)) { //true to set the size to the current progress
				Serial.printf("\r\nFirmware update successful: %u bytes\r\nRebooting...\r\n", upload.totalSize);
			} else {
				Update.printError(Serial);
			}
		}
	});
	*/

	// Index page
	OTAServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(200, "text/html", F("<h1>WebOTA</h1>"));
	});

	// Handle 404s
	OTAServer.onNotFound(handle404);

	//server->begin();

	return 1;
}

// If the MCU is in a delay() it cannot respond to HTTP OTA requests
// We do a "fake" looping delay and listen for incoming HTTP requests while waiting
void WebOTA::delay(unsigned int ms) {
	::delay(5);
}

void WebOTA::set_custom_html(char const * const html) {
	this->custom_html = html;
}
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

int init_mdns(const char *host) {
	// Use mdns for host name resolution
	if (!MDNS.begin(host)) {
		Serial.println("Error setting up MDNS responder!");

		return 0;
	}

	Serial.printf("mDNS started : %s.local\r\n", host);

	webota.mdns = host;

	return 1;
}

String ip2string(IPAddress ip) {
	String ret = String(ip[0]) + "." +  String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);

	return ret;
}

int init_wifi(const char *ssid, const char *password, const char *mdns_hostname) {
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	Serial.println("");
	Serial.print("Connecting to Wifi");

	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.printf("Connected to '%s'\r\n\r\n",ssid);

	String ipaddr = ip2string(WiFi.localIP());
	Serial.printf("IP address   : %s\r\n", ipaddr.c_str());
	Serial.printf("MAC address  : %s \r\n", WiFi.macAddress().c_str());

	init_mdns(mdns_hostname);

	return 1;
}
