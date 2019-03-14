// Arduino build process info: https://github.com/arduino/Arduino/wiki/Build-Process

const char *WEBOTA_VERSION = "0.1.3";

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WebServer.h>

// Index page
const char* indexPage = "<h1>ESP32 Index</h1>";

// WebOTA Page
String get_ota_html() {
	String ota_html = "";

	ota_html += "<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js\"></script>\n";
	ota_html += "\n";
	ota_html += "<h1>WebOTA Version: " + (String) WEBOTA_VERSION + "</h1>\n";
	ota_html += "\n";
	ota_html += "<form method=\"POST\" action=\"#\" enctype=\"multipart/form-data\" id=\"upload_form\">\n";
	ota_html += "    <input type=\"file\" name=\"update\">\n";
	ota_html += "    <input type=\"submit\" value=\"Update\">\n";
	ota_html += "</form>\n";
	ota_html += "\n";
	ota_html += "<div id=\"prg_wrap\" style=\"border: 0px solid; width: 100%;\">\n";
	ota_html += "   <div id=\"prg\" style=\"text-shadow: 2px 2px 3px black; padding: 5px 0; display: none; border: 1px solid #008aff; background: #002180; text-align: center; color: white;\"></div>\n";
	ota_html += "</div>\n";
	ota_html += "\n";
	ota_html += "<script>\n";
	ota_html += "$(\"form\").submit(function(e){\n";
	ota_html += "    e.preventDefault();\n";
	ota_html += "    var form = $(\"#upload_form\")[0];\n";
	ota_html += "    var data = new FormData(form);\n";
	ota_html += "\n";
	ota_html += "    $.ajax({\n";
	ota_html += "        url: \"/update\",\n";
	ota_html += "        type: \"POST\",\n";
	ota_html += "        data: data,\n";
	ota_html += "        contentType: false,\n";
	ota_html += "        processData:false,\n";
	ota_html += "        xhr: function() {\n";
	ota_html += "            var xhr = new window.XMLHttpRequest();\n";
	ota_html += "            xhr.upload.addEventListener(\"progress\", function(evt) {\n";
	ota_html += "                if (evt.lengthComputable) {\n";
	ota_html += "                    var per = Math.round((evt.loaded / evt.total) * 100);\n";
	ota_html += "                    $(\"#prg\").html(per + \"%\").css(\"width\", per + \"%\").show();\n";
	ota_html += "                }\n";
	ota_html += "            }, false);\n";
	ota_html += "\n";
	ota_html += "            return xhr;\n";
	ota_html += "        },\n";
	ota_html += "        success:function(d, s) {\n";
	ota_html += "            console.log(\"success!\")\n";
	ota_html += "        },\n";
	ota_html += "        error: function (a, b, c) {}\n";
	ota_html += "    });\n";
	ota_html += "});\n";
	ota_html += "</script>\n";

	return ota_html;
}

String ip2string(IPAddress ip) {
	String ret = String(ip[0]) + "." +  String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);

	return ret;
}

int init_mdns(const char *host) {
	/*use mdns for host name resolution*/
	if (!MDNS.begin(host)) { //http://esp32.local
		Serial.println("Error setting up MDNS responder!");
		while (1) {
			delay(1000);
		}
	}

	Serial.printf("mDNS started : %s\r\n", host);
}

int init_wifi(const char *ssid, const char *password, const char *mdns_hostname) {
	// Connect to WiFi network
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

	//String macAddr = mac2string(WiFi.macAddress());
	Serial.printf("MAC address  : %s \r\n", WiFi.macAddress().c_str());

	init_mdns(mdns_hostname);

	Serial.printf("WebOTA url   : http://%s.local:%d/webota\r\n\r\n", mdns_hostname, 8080);
}

int init_web_ota(WebServer *server) {
	// Login page
	server->on("/", HTTP_GET, [server]() {
		server->send(200, "text/html", indexPage);
	});

	// Upload firmware page
	server->on("/webota", HTTP_GET, [server]() {
		String ota_html = get_ota_html();
		server->send(200, "text/html", ota_html.c_str());
	});

	// Handling uploading firmware file
	server->on("/update", HTTP_POST, [server]() {
		server->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
		ESP.restart();
	}, [server]() {
		static uint32_t next = 102400;

		HTTPUpload& upload = server->upload();
		if (upload.status == UPLOAD_FILE_START) {
			Serial.printf("Firmware update initiated: %s\r\n", upload.filename.c_str());
			if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
				Update.printError(Serial);
			}
		} else if (upload.status == UPLOAD_FILE_WRITE) {
			/* flashing firmware to ESP*/
			if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
				Update.printError(Serial);
			}

			if (upload.totalSize >= next) {
				Serial.printf("%dk ", next / 1024);
				next += 102400;
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

// If the MCU is in a delay() it cannot respond to HTTP OTA requests
// We do a "fake" looping delay and listen for incoming HTTP requests while waiting
void webota_delay(int ms) {
	int last = millis();

	extern WebServer OTAServer;

	while ((millis() - last) < ms) {
		OTAServer.handleClient();
		delay(5);
	}
}

// Accept the HTTP server as a pointer
void webota_delay(int ms, WebServer *HTTPServer) {
	int last = millis();

	while ((millis() - last) < ms) {
		HTTPServer->handleClient();
		delay(5);
	}
}
