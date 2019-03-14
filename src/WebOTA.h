#include <WebServer.h>

int init_wifi(const char *ssid, const char *password, const char *mdns_hostname);
String ip2string(IPAddress ip);
int init_mdns(const char *host);
int init_web_ota(WebServer *server);
