#include <WebServer.h>

int init_wifi(const char *ssid, const char *password, const char *mdns_hostname);
int init_mdns(const char *host);
int init_web_ota(WebServer *server);

void webota_delay(int ms);
void webota_delay(int ms, WebServer *HTTPServer);
