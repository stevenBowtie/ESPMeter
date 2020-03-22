#include <Arduino.h>
//#include <WiFi.h>
//#include <AsyncTCP.h>
#include "serverConfig.h"

//AsyncWebServer server(80);

const char* ssid = "ESPMeter";
const char* password = "nopassword";

void setup() {

    Serial.begin(115200);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();

    init_server_callbacks();
    server.begin();
}

void loop() {
}
