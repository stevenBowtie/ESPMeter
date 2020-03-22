#ifndef serverConfig
#define serverConfig
#endif
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#ifndef AsyncWebServer
#include <ESPAsyncWebServer.h>
#endif

const char* PARAM_MESSAGE = "message";

AsyncWebServer server(80);

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void init_server_callbacks(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "Hello, world");
  });

  // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String message;
      if (request->hasParam(PARAM_MESSAGE)) {
          message = request->getParam(PARAM_MESSAGE)->value();
      } else {
          message = "No message sent";
      }
      request->send(200, "text/plain", "Hello, GET: " + message);
  });

  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on("/spiffs", HTTP_GET, [](AsyncWebServerRequest *request){
      String message;
      File file = SPIFFS.open("/spiffs");
      if(!file){ message="Failed to open file for reading"; }

      while(file.available()){
        message.concat( file.read() );
      }
      file.close();
      request->send(200, "text/plain", "Hello, POST: " + message);
  });

  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
      String message;
      if (request->hasParam(PARAM_MESSAGE, true)) {
          message = request->getParam(PARAM_MESSAGE, true)->value();
      } else {
          message = "No message sent";
      }
      request->send(200, "text/plain", "Hello, POST: " + message);
  });
  
  server.onNotFound(notFound);
  Serial.println("Server Configured");
}
