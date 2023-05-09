#ifndef serverConfig
#define serverConfig
#endif
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#ifndef AsyncWebServer
#include <ESPAsyncWebSrv.h>
#endif
#ifndef config_h
#include "config.h"
#endif
#include <ArduinoJson.h>
#include "src/battcon.h"

const char* PARAM_MESSAGE = "message";
const char * HOST = "ESPmeter";
char baseMacChr[9] = {0};

AsyncWebServer server(80);
extern long chan0;
extern IPAddress apIP;
extern IPAddress staIP;
extern BattCon batt;

StaticJsonDocument<2048> json_config;
String json_str = "";

bool check_config_exists( String fn ){
  File file = SPIFFS.open( fn, "r" );
  if( file ){ 
    file.close();
    return 1; 
  }
  else{
    File file = SPIFFS.open( fn, "w" );
    const String jstr = "{\"charge_voltage\": 14.4,\"charge_current\": 0.1,\"charge_timeout\": 3600,"
    "\"discharge_voltage\": 11.1,\"discharge_current\": 1.0,\"charge_begin\": 0,\"discharge_time\": 3600,"
    "\"dwell_time\": 3600,\"dwell_time_threshold\": 0,\"dwell_ended\": 0,\"dwell_poll_rate\": 60000,"
    "\"dwell_voltage_threshold\": 12.0}";
    file.print( jstr );
    file.close();
  }
  return 0;
}

bool load_config( String fn, JsonDocument& jdoc, String & jstr ){
  File file = SPIFFS.open( fn, "r" );
  if( file ){
    jstr="";
    jstr = file.readString();
    Serial.println( fn );
    Serial.println( jstr );
    DeserializationError err = deserializeJson( jdoc, jstr );
    Serial.println( err.c_str() );
    file.close();
    return 1;
  }
  Serial.println("Failed to load: " + fn );
  return 0;
}

bool save_config( String fn, JsonDocument& jdoc, String & jstr ){
  File file = SPIFFS.open( fn, "w" );
  if( file ){
    jstr="";
    serializeJson( jdoc, jstr );
    file.print( jstr );
    file.close();
    Serial.println( fn );
    Serial.println( jstr );
    return 1;
  }
  return 0;
}

void update_from_json(){
  //Battery Conditioning
  batt.charge_voltage = json_config["charge_voltage"];
  batt.charge_current = json_config["charge_current"];
  batt.charge_timeout = json_config["charge_timeout"];
  batt.discharge_voltage = json_config["discharge_voltage"];
  batt.discharge_current = json_config["discharge_current"];
  batt.discharge_current = json_config["discharge_current"];        // Discharge termination current
  batt.charge_begin = json_config["charge_begin"];                  // Discharge time limit
  batt.discharge_time = json_config["discharge_time"];              // Discharge time limit
  batt.dwell_time  = json_config["dwell_time"];                     // How long to wait in Dwell period
  batt.dwell_time_threshold = json_config["dwell_time_threshold"] ; // How long to wait in Dwell period
  batt.dwell_ended = json_config["dwell_ended"] ;                     // Time that dwell period ended
  batt.dwell_poll_rate = json_config["dwell_poll_rate"];              // How often to check the change in voltage
	batt.dwell_voltage_threshold = json_config["dwell_voltage_threshold"] ;  // Minimum voltage change to end dwell 
}

void server_setup(){
  check_config_exists("/json_config.json");
  load_config("/json_config.json", json_config, json_str);
}
  
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

  server.on("/batt_status", HTTP_GET, [](AsyncWebServerRequest *request){
      String message;
      message = batt.current_state;
      request->send(200, "text/json", "{\"readings\":["+message+"]}");
  });
  server.on("/ip", HTTP_GET, [](AsyncWebServerRequest *request){
      String staIP = WiFi.localIP().toString();
      String apIP = WiFi.softAPIP().toString();
      request->send(200, "text/json", "{\"STA_IP\":\""+ staIP +"\",\"AP_IP\":\""+ apIP +"\"}");
  });

  server.on("/config/get", HTTP_GET, [](AsyncWebServerRequest *request){
    String recvd ="";
    serializeJson( json_config, recvd );
    request->send(200, "text/json", recvd);
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
