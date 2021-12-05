#ifndef serverConfig
#define serverConfig
#endif
#include <WiFi.h>
//#include <AsyncTCP.h>
#include <SPIFFS.h>
#ifndef AsyncWebServer
//#include <ESPAsyncWebServer.h>
#endif
#ifndef config_h
#include "config.h"
#endif

const char* PARAM_MESSAGE = "message";
const char * HOST = "ESPmeter";
char baseMacChr[9] = {0};

AsyncWebServer server(80);
extern long chan0;
extern IPAddress apIP;
extern IPAddress staIP;

void getMacName(){
	uint8_t baseMac[6];
	esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
	sprintf(baseMacChr, "%s%02X%02X", HOST, baseMac[4], baseMac[5]);
}

String convertPointer(char *startingPointer){
  String conversion;
  while(*startingPointer){
    conversion.concat(*startingPointer);
    startingPointer++;
  }
  return conversion;
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void init_server_callbacks(){
  //server.serveStatic("/", SPIFFS, "/spiffs.htm");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS,"/spiffs.htm", "text/html");
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS,"/favicon.ico", "image/x-icon");
  });

  server.on("/reading", HTTP_GET, [](AsyncWebServerRequest *request){
      String message;
      message=chan0;
      request->send(200, "text/json", "{\"readings\":["+message+"]}");
  });

  server.on("/ip", HTTP_GET, [](AsyncWebServerRequest *request){
      String staIP = WiFi.localIP().toString();
      String apIP = WiFi.softAPIP().toString();
      request->send(200, "text/json", "{\"STA_IP\":\""+ staIP +"\",\"AP_IP\":\""+ apIP +"\"}");
  });

  server.on("/config", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(SPIFFS,"/config.htm", "text/html");
  });

  server.on("/save", HTTP_POST, [] (AsyncWebServerRequest *request) {
    Serial.print("Request received: params ");
    int params = request->params();
    Serial.println(params);
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    request->send(200, "text/plain", "ACK");
  });

  server.on("/spiffs", HTTP_GET, [](AsyncWebServerRequest *request){
    /*  String message;
      File file = SPIFFS.open("/spiffs.htm");
      if(!file){ message="Failed to open file for reading"; }

      while(file.available()){
        message.concat( file.read() );
      }
      file.close(); */
      request->send(SPIFFS,"/spiffs.htm", "text/html");
  });

  server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS,"/config.json", "text/html");
  });

  server.onNotFound(notFound);
  Serial.println("Server Configured");
}

void print_spiffs(){
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
 
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
 
  while(file){
 
      Serial.print("FILE: ");
      Serial.println(file.name());
 
      file = root.openNextFile();
  }
}
