#ifndef config
#define config
#ifndef SPIFFS
#include <SPIFFS.h>
#endif
#include <ArduinoJson.h>

const char * wifi_mode;
const char * wifiAPssid;
const char * wifiAPpass;
StaticJsonDocument<256> cfg;

bool load_config(){
  Serial.println("Loading Config...");
  if (!SPIFFS.begin(true)) { 
    Serial.println("SPIFFS failure");
    return 0; 
  }
  File file = SPIFFS.open("/config.json");
  if(!file){ 
    Serial.println("Failed to open config"); 
    return 0; 
  } 
  //DynamicJsonDocument cfg(200);
  DeserializationError error = deserializeJson(cfg, file);
  if (error){ Serial.println(error.c_str()); }
  wifi_mode = cfg["wifi_mode"];
  wifiAPssid = cfg["wifiAPssid"];
  wifiAPpass = cfg["wifiAPpass"];
  Serial.println(wifi_mode);
  return 1;
}
#endif
