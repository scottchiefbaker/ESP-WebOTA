#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266WebServer.h>
#endif
#ifdef ESP32
#include <WebServer.h>
#endif

typedef enum {
  OTA_AUTH_ERROR,
  OTA_BEGIN_ERROR,
  OTA_CONNECT_ERROR,
  OTA_RECEIVE_ERROR,
  OTA_END_ERROR
} ota_error_t;

class WebOTA {
	public:
		unsigned int port;
		String path = "";
		String mdns = "";

		int init(const unsigned int port, const char *path);
		int init(const unsigned int port);
		int init();
		void delay(unsigned int ms);

#ifdef ESP8266
		int add_http_routes(ESP8266WebServer *server, const char *path);
#endif
#ifdef ESP32
		int add_http_routes(WebServer *server, const char *path);
#endif

		int handle();

		void set_custom_html(char const * const html);
		void useAuth(const char* user, const char* password);

		typedef std::function<void(void)> THandlerFunction;
    	typedef std::function<void(ota_error_t)> THandlerFunction_Error;
    	typedef std::function<void(unsigned int, unsigned int)> THandlerFunction_Progress;

		//This callback will be called when OTA connection has begun
    	WebOTA& onStart(THandlerFunction fn);

    	//This callback will be called when OTA has finished
    	WebOTA& onEnd(THandlerFunction fn);

    	//This callback will be called when OTA encountered Error
    	WebOTA& onError(THandlerFunction_Error fn);

    	//This callback will be called when OTA is receiving data
    	WebOTA& onProgress(THandlerFunction_Progress fn);

	private:
		bool init_has_run;
		char const * custom_html = NULL;
		String get_ota_html();
		String human_time(uint32_t sec);
		String get_board_type();
		long max_sketch_size();
		THandlerFunction _start_callback;
    	THandlerFunction _end_callback;
    	THandlerFunction_Error _error_callback;
    	THandlerFunction_Progress _progress_callback;
};

int init_wifi(const char *ssid, const char *password, const char *mdns_hostname);

extern WebOTA webota;
