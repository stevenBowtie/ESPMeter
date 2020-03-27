#include <Arduino.h>
#include "serverConfig.h"
#include "config.h"

const char* ap_ssid = "ESPMeter";
const char* ap_pass = "nopassword";
const char* sta_ssid = "PWNZ0RZ";
//const char* sta_pass = "supersecretpassword";
#include "password.h"

int avg_factor = 1000;
int analogAvg = 0;

byte flag = 1;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(sta_ssid,sta_pass);
  WiFi.softAP(ap_ssid, ap_pass);
  IPAddress apIP = WiFi.softAPIP();
  IPAddress staIP = WiFi.localIP();
  Serial.print("AP: ");
  Serial.println(apIP);
  Serial.print("STA: ");
  Serial.println(staIP);
  load_config();   
  server.begin();
  print_spiffs();
  init_server_callbacks();
}

void loop() {
  analogAvg=((analogAvg*avg_factor)+analogRead(A4))/(avg_factor+1);
  if( millis() % 1000 == 1 ){
    if( flag ){
  Serial.print("STA: ");
  Serial.println(WiFi.localIP());
      Serial.println( analogAvg );
      flag = 0;
    }
  }
  else{ flag = 1; }
}
