// Arduino build process info: https://github.com/arduino/Arduino/wiki/Build-Process

const char *WEBOTA_VERSION = "0.1.5";
bool INIT_RUN              = false;

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

////////////////////////////////////////////////////////////////////////////

int WebOTA::init(const unsigned int port, const char *path) {
	this->port = port;
	this->path = path;

	if (INIT_RUN == true) {
		return 0;
	}

	add_http_routes(&OTAServer, path);
	OTAServer.begin(port);

	Serial.printf("WebOTA url   : http://%s.local:%d%s\r\n\r\n", this->mdns.c_str(), port, path);

	// Store that init has already run
	INIT_RUN = true;

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
	static bool init_run = false;

	if (INIT_RUN == false) {
		WebOTA::init();
	}

	OTAServer.handleClient();
	MDNS.update();
}

long WebOTA::max_sketch_size() {
	long ret = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;

	return ret;
}

#ifdef ESP8266
int WebOTA::add_http_routes(ESP8266WebServer *server, const char *path) {
#endif
#ifdef ESP32
int WebOTA::add_http_routes(WebServer *server, const char *path) {
#endif
	// Index page
	server->on("/", HTTP_GET, [server]() {
		server->send(200, "text/html", "<h1>WebOTA</h1>");
	});

	// Upload firmware page
	server->on(path, HTTP_GET, [server,this]() {
		String ota_html = this->get_ota_html();
		server->send(200, "text/html", ota_html.c_str());
	});

	// Handling uploading firmware file
	server->on(path, HTTP_POST, [server,this]() {
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

	server->begin();
}

// Get the HTML for the sketch upload page
String WebOTA::get_ota_html() {
	String ota_html = "";

	ota_html += "<h1>WebOTA Version: " + (String)WEBOTA_VERSION + "</h1>\n";
	ota_html += "\n";
	ota_html += "<form method=\"POST\" action=\"#\" enctype=\"multipart/form-data\" id=\"upload_form\">\n";
	ota_html += "    <input type=\"file\" name=\"update\" id=\"file\">\n";
	ota_html += "    <input type=\"submit\" value=\"Update\">\n";
	ota_html += "</form>\n";
	ota_html += "\n";
	ota_html += "<div style=\"font-size: 75%;\">Max sketch size: " + (String)this->max_sketch_size() + "</div>\n";
	ota_html += "<div id=\"prg_wrap\" style=\"border: 0px solid; width: 100%;\">\n";
	ota_html += "   <div id=\"prg\" style=\"text-shadow: 2px 2px 3px black; padding: 5px 0; display: none; border: 1px solid #008aff; background: #002180; text-align: center; color: white;\"></div>\n";
	ota_html += "</div>\n";
	ota_html += "\n";
	ota_html += "<script>\n";
	ota_html += "var domReady = function(callback) {\n";
	ota_html += "	document.readyState === \"interactive\" || document.readyState === \"complete\" ? callback() : document.addEventListener(\"DOMContentLoaded\", callback);\n";
	ota_html += "};\n";
	ota_html += "\n";
	ota_html += "domReady(function() {\n";
	ota_html += "	var myform = document.getElementById('upload_form');\n";
	ota_html += "	var filez  = document.getElementById('file');\n";
	ota_html += "\n";
	ota_html += "	myform.onsubmit = function(event) {\n";
	ota_html += "		event.preventDefault();\n";
	ota_html += "\n";
	ota_html += "		var formData = new FormData();\n";
	ota_html += "		var file     = filez.files[0];\n";
	ota_html += "\n";
	ota_html += "		if (!file) { return false; }\n";
	ota_html += "\n";
	ota_html += "		formData.append(\"files\", file, file.name);\n";
	ota_html += "\n";
	ota_html += "		var xhr = new XMLHttpRequest();\n";
	ota_html += "		xhr.upload.addEventListener(\"progress\", function(evt) {\n";
	ota_html += "			if (evt.lengthComputable) {\n";
	ota_html += "				var per = Math.round((evt.loaded / evt.total) * 100);\n";
	ota_html += "				var prg = document.getElementById('prg');\n";
	ota_html += "\n";
	ota_html += "				prg.innerHTML     = per + \"%\"\n";
	ota_html += "				prg.style.width   = per + \"%\"\n";
	ota_html += "				prg.style.display = \"block\"\n";
	ota_html += "			}\n";
	ota_html += "		}, false);\n";
	ota_html += "		xhr.open('POST', location.href, true);\n";
	ota_html += "\n";
	ota_html += "		// Set up a handler for when the request finishes.\n";
	ota_html += "		xhr.onload = function () {\n";
	ota_html += "			if (xhr.status === 200) {\n";
	ota_html += "				//alert('Success');\n";
	ota_html += "			} else {\n";
	ota_html += "				//alert('An error occurred!');\n";
	ota_html += "			}\n";
	ota_html += "		};\n";
	ota_html += "\n";
	ota_html += "		xhr.send(formData);\n";
	ota_html += "   }\n";
	ota_html += "});\n";
	ota_html += "</script>\n";

	return ota_html;
}

// If the MCU is in a delay() it cannot respond to HTTP OTA requests
// We do a "fake" looping delay and listen for incoming HTTP requests while waiting
void WebOTA::delay(int ms) {
	int last = millis();

	while ((millis() - last) < ms) {
		OTAServer.handleClient();
		delay(5);
	}
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
}
