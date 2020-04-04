#include <Arduino.h>
#include "serverConfig.h"
#include "config.h"

//ADS1115
#include "lib\I2Cdev.h"
#include "lib\I2Cdev.cpp"
#include "lib\ADS1115.h"
#include "lib\ADS1115.cpp"

const char* ap_ssid = "ESPMeter";
const char* ap_pass = "nopassword";
const char* sta_ssid = "PWNZ0RZ";
//const char* sta_pass = "supersecretpassword";
#include "password.h"

int avg_factor = 1000;
int analogAvg = 0;

byte flag = 1;

ADS1115 adc(ADS1115_DEFAULT_ADDRESS);
const int ads_SDA = 14;
const int ads_SCL = 27;
const int alertReadyPin = 26;
volatile bool rdy_state = 0;
float reading = 0;


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

  //ADC config
  Wire.begin( ads_SDA, ads_SCL );
  adc.initialize();
  adc.setMode(ADS1115_MODE_SINGLESHOT);
  adc.setRate(ADS1115_RATE_860);
  adc.setGain(ADS1115_PGA_1P024);
  adc.setComparatorQueueMode(ADS1115_COMP_QUE_ASSERT1);
  adc.setConversionReadyPinMode();
  adc.setComparatorPolarity(0);
  #ifdef ADS1115_SERIAL_DEBUG
  adc.showConfigRegister();
  Serial.print("HighThreshold="); Serial.println(adc.getHighThreshold(),BIN);
  Serial.print("LowThreshold="); Serial.println(adc.getLowThreshold(),BIN);
  #endif
  pinMode(alertReadyPin,INPUT_PULLUP);
  attachInterrupt( alertReadyPin, rdy_interrupt, FALLING );
}

void loop() {
  analogAvg=((analogAvg*avg_factor)+analogRead(A4))/(avg_factor+1);
  ADCpoll(); 
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

void pollAlertReadyPin() {
  while( !rdy_state ){
  }
  reading = adc.getMilliVolts(false);
  rdy_state = 0;
}

void rdy_interrupt(){
  rdy_state = 1;
}

void ADCpoll(){
       
    // The below method sets the mux and gets a reading.
    adc.setMultiplexer(ADS1115_MUX_P0_NG);
    adc.triggerConversion();
    pollAlertReadyPin();
    Serial.print("A0: "); 
    Serial.print(reading);    

    adc.setMultiplexer(ADS1115_MUX_P1_NG);
    //adc.triggerConversion();
    pollAlertReadyPin();
    Serial.print(",A1: "); 
    Serial.print(reading);
    
    adc.setMultiplexer(ADS1115_MUX_P2_NG);
    //adc.triggerConversion();
    pollAlertReadyPin();
    Serial.print(",A2: "); 
    Serial.print(reading);
    
    adc.setMultiplexer(ADS1115_MUX_P3_NG);
    //adc.triggerConversion();
    pollAlertReadyPin();
    Serial.print(",A3: "); 
    Serial.println(reading); 
}
