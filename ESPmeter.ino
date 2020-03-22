#include <Arduino.h>
//#include <WiFi.h>
//#include <AsyncTCP.h>
#include "serverConfig.h"

//AsyncWebServer server(80);

const char* ssid = "ESPMeter";
const char* password = "nopassword";

int avg_factor = 1000;
int analogAvg = 0;
void setup() {

    Serial.begin(115200);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();

    init_server_callbacks();
    server.begin();
    print_spiffs();
   
}

void loop() {
  analogAvg=((analogAvg*avg_factor)+analogRead(A4))/(avg_factor+1);
  Serial.println(analogAvg);
}
