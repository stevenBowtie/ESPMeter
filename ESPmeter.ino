#include <Arduino.h>
#include "serverConfig.h"
#include "config.h"

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
  load_config();   
}

void loop() {
  analogAvg=((analogAvg*avg_factor)+analogRead(A4))/(avg_factor+1);
  if( millis() % 1000 == 0 ){
    Serial.println( analogAvg );
  }
}
