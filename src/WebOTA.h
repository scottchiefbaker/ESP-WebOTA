#include <WebServer.h>

extern const char *WEBOTA_VERSION;
extern WebServer OTAServer;

int init_wifi(const char *ssid, const char *password, const char *mdns_hostname);
int init_wifi(const char *ssid, const char *password);
int init_mdns(const char *host);

void init_webota(const int port, const char *path);
void init_webota(const int port);
void init_webota();

int handle_webota();

void webota_delay(int ms);
