#include <Arduino.h>
#include <ESPAsyncWebSrv.h>

class WebOTA {
	public:
		unsigned int port;
		String path = "";
		String mdns = "";

		int init(const unsigned int port, const char *path);
		int init(const unsigned int port);
		int init();
		void delay(unsigned int ms);

		int add_http_routes(AsyncWebServer *server, const char *path);

		int handle();

		void set_custom_html(char const * const html);
		long max_sketch_size();

	private:
		bool init_has_run;
		char const * custom_html = NULL;
		String get_board_type();
};

int init_wifi(const char *ssid, const char *password, const char *mdns_hostname);

extern WebOTA webota;
