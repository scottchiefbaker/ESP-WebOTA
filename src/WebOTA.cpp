// Arduino build process info: https://github.com/arduino/Arduino/wiki/Build-Process

#define WEBOTA_VERSION "0.1.6"

#include "WebOTA.h"
#include <Arduino.h>
#include <WiFiClient.h>

#ifdef ESP32
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WiFi.h>

WebServer OTAServer(9999);
#endif

#ifdef ESP8266
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

ESP8266WebServer OTAServer(9999);
#endif

WebOTA webota;

char WWW_USER[16]       = "";
char WWW_PASSWORD[16]   = "";
const char* WWW_REALM   = "WebOTA";
// the Content of the HTML response in case of Unautherized Access Default:empty
String authFailResponse = "Auth Fail";

////////////////////////////////////////////////////////////////////////////

int WebOTA::init(const unsigned int port, const char *path) {
	this->port = port;
	this->path = path;

	// Only run this once
	if (this->init_has_run) {
		return 0;
	}

	add_http_routes(&OTAServer, path);

#ifdef ESP32
	// https://github.com/espressif/arduino-esp32/issues/7708
	// Fix some slowness
	OTAServer.enableDelay(false);
#endif
	OTAServer.begin(port);

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

void WebOTA::useAuth(const char* user, const char* password) {
	strncpy(WWW_USER, user, sizeof(WWW_USER) - 1);
	strncpy(WWW_PASSWORD, password, sizeof(WWW_PASSWORD) - 1);

	//Serial.printf("Set auth '%s' / '%s' %d\n", user, password, len);
}

int WebOTA::handle() {
	// If we haven't run the init yet run it
	if (!this->init_has_run) {
		WebOTA::init();
	}

	OTAServer.handleClient();
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

	<script src="main.js"></script>
	<link rel="stylesheet" href="main.css">
</head>
<body>
	<h1>WebOTA version %s</h1>

	<form method="POST" action="#" enctype="multipart/form-data" id="upload_form">
		<div><input class="" type="file" name="update" id="file"></div>

		<div><input class="btn" type="submit" value="Upload"></div>
	</form>

	<p>
		<b>Board:</b> %s<br />
		<b>MAC:</b> %s<br />
		<b>Uptime:</b> %s<br />
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

String WebOTA::get_board_type() {

// More information: https://github.com/search?q=repo%3Aarendst%2FTasmota%20esp32s2&type=code

#if defined(ESP8266)
	String BOARD_NAME = "ESP8266";
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
	String BOARD_NAME = "ESP32-S2";
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
	String BOARD_NAME = "ESP32-S3";
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
	String BOARD_NAME = "ESP32-C3";
#elif defined(CONFIG_IDF_TARGET_ESP32)
	String BOARD_NAME = "ESP32";
#elif defined(CONFIG_ARDUINO_VARIANT)
	String BOARD_NAME = CONFIG_ARDUINO_VARIANT;
#else
	String BOARD_NAME = "Unknown";
#endif

	return BOARD_NAME;
}

String get_mac_address() {
	uint8_t mac[6];

	// Put the addr in mac
	WiFi.macAddress(mac);

	// Build a string and return it
	char buf[20] = "";
	snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

	String ret = buf;

	return ret;
}

#ifdef ESP32
int8_t check_auth(WebServer *server) {
#endif
#ifdef ESP8266
int8_t check_auth(ESP8266WebServer *server) {
#endif
	// If we have a user and a password we check digest auth
	bool use_auth = (strlen(WWW_USER) && strlen(WWW_PASSWORD));
	if (!use_auth) {
		return 1;
	}

	if (!server->authenticate(WWW_USER, WWW_PASSWORD)) {
		//Basic Auth Method
		//return server.requestAuthentication(BASIC_AUTH, WWW_REALM, authFailResponse);

		// Digest Auth
		server->requestAuthentication(DIGEST_AUTH, WWW_REALM, authFailResponse);

		return 0;
	}

	return 2;
}

#ifdef ESP8266
int WebOTA::add_http_routes(ESP8266WebServer *server, const char *path) {
#endif
#ifdef ESP32
int WebOTA::add_http_routes(WebServer *server, const char *path) {
#endif
	// Index page
	server->on("/", HTTP_GET, [server]() {
		check_auth(server);

		server->send(200, "text/html", F("<h1 style=\"font-family: sans-serif;\">Arduino WebOTA</h1>"));
	});

	// Upload firmware page
	server->on(path, HTTP_GET, [server,this]() {
		check_auth(server);

		String html = "";
		if (this->custom_html != NULL) {
			html = this->custom_html;
		} else {
			//uint32_t maxSketchSpace = this->max_sketch_size();

			String uptime_str = human_time(millis() / 1000);
			String board_type = webota.get_board_type();
			String mac_addr   = get_mac_address();

			char buf[1024];
			snprintf_P(buf, sizeof(buf), INDEX_HTML, WEBOTA_VERSION, board_type, mac_addr.c_str(), uptime_str.c_str());

			html = buf;
		}

		server->send_P(200, "text/html", html.c_str());
	});

	// Handling uploading firmware file
	server->on(path, HTTP_POST, [server,this]() {
		check_auth(server);

		server->send(200, "text/plain", (Update.hasError()) ? "Update: fail\n" : "Update: OK!\n");
		delay(500);
		ESP.restart();
	}, [server,this]() {
		HTTPUpload& upload = server->upload();

		if (upload.status == UPLOAD_FILE_START) {
			Serial.printf("Firmware update initiated: %s\r\n", upload.filename.c_str());

			//uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
			uint32_t maxSketchSpace = this->max_sketch_size();

			if (!Update.begin(maxSketchSpace)) { //start with max available size
				Update.printError(Serial);
			}
		} else if (upload.status == UPLOAD_FILE_WRITE) {
			/* flashing firmware to ESP*/
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

	// FILE: main.js
	server->on("/main.js", HTTP_GET, [server]() {
		server->send_P(200, "application/javascript", MAIN_JS);
	});

	// FILE: main.css
	server->on("/main.css", HTTP_GET, [server]() {
		server->send_P(200, "text/css", MAIN_CSS);
	});

	// FILE: favicon.ico
	server->on("/favicon.ico", HTTP_GET, [server]() {
		server->send(200, "image/vnd.microsoft.icon", "");
	});

	server->begin();

	return 1;
}

// If the MCU is in a delay() it cannot respond to HTTP OTA requests
// We do a "fake" looping delay and listen for incoming HTTP requests while waiting
void WebOTA::delay(unsigned int ms) {
	// Borrowed from mshoe007 @ https://github.com/scottchiefbaker/ESP-WebOTA/issues/8
	decltype(millis()) last = millis();

	while ((millis() - last) < ms) {
		OTAServer.handleClient();
		::delay(5);
	}
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

String WebOTA::human_time(uint32_t sec) {
    int days = (sec / 86400);
    sec = sec % 86400;
    int hours = (sec / 3600);
    sec = sec % 3600;
    int mins  = (sec / 60);
    sec = sec % 60;

	char buf[24] = "";
	if (days) {
        snprintf(buf, sizeof(buf), "%d days %d hours\n", days, hours);
    } else if (hours) {
        snprintf(buf, sizeof(buf), "%d hours %d minutes\n", hours, mins);
    } else {
        snprintf(buf, sizeof(buf), "%d minutes %d seconds\n", mins, sec);
    }

	String ret = buf;

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
