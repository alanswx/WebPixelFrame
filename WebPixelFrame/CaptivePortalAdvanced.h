#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
//#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

void handleRoot(void);
boolean captivePortal();
 void handleWifiJSON();
void handleWifiSave();
void handleWifi();
void handleNotFound();
boolean isIp(String str);
String toStringIp(IPAddress ip);
void loadCredentials();
void saveCredentials();
//void setupCaptive(ESP8266WebServer *server);
void loopCaptive();



