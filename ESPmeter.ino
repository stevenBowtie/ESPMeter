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

int avg_factor = 60;
float analogAvg = 0;
long chan0 = 0;

byte flag = 1;

ADS1115 adc(ADS1115_DEFAULT_ADDRESS);
const int ads_SDA = 14;
const int ads_SCL = 27;
const int alertReadyPin = 26;
bool rdy_state = 1;
float reading = 0;
float thisSample;

uint8_t current_channel = 0;
float ads_readings[] = { 0, 0, 0, 0 };
const uint8_t muxBits[] = { ADS1115_MUX_P0_NG, ADS1115_MUX_P1_NG, ADS1115_MUX_P2_NG, ADS1115_MUX_P3_NG };

unsigned long cycle_count = 0;

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
  #ifdef ADS1115_SERIAL_DEBUG
  adc.showConfigRegister();
  Serial.print("HighThreshold="); Serial.println(adc.getHighThreshold(),BIN);
  Serial.print("LowThreshold="); Serial.println(adc.getLowThreshold(),BIN);
  #endif
  pinMode(alertReadyPin,INPUT_PULLUP);
  attachInterrupt( alertReadyPin, rdy_interrupt, FALLING );
}

void loop() {
  //analogAvg=((analogAvg*avg_factor)+analogRead(A4))/(avg_factor+1);
  chan0 =sqrt(ads_readings[0])-1648;
  ADCpoll(); 
  if( millis() % 1000 == 1 ){
    if( flag ){
     setADCconfig(); 
      flag = 0;
    }
  }
  else{ flag = 1; }
}

void rdy_interrupt(){
  rdy_state = 1;
}

void ADCpoll(){       
  if(rdy_state || cycle_count>1000){
    rdy_state = 0;
    thisSample = adc.getMilliVolts(false);
    analogAvg = ads_readings[current_channel];
    ads_readings[current_channel] = 
      ((analogAvg*avg_factor)+pow(thisSample,2)) / (avg_factor+1);
    //current_channel = (current_channel+1)%4;
    adc.setMultiplexer( muxBits[current_channel] );
    adc.triggerConversion();
  //Serial.println(current_channel);

    for(int i=0; i<4; i++){
      Serial.print(sqrt(ads_readings[i]));
      Serial.print(",");
    }
  Serial.println(cycle_count);
  cycle_count=0;
  }
  cycle_count++;
}

void setADCconfig(){
  adc.setMode(ADS1115_MODE_SINGLESHOT);
  adc.setRate(ADS1115_RATE_860);
  adc.setGain(ADS1115_PGA_4P096);
  adc.setComparatorQueueMode(ADS1115_COMP_QUE_ASSERT1);
  adc.setConversionReadyPinMode();
  adc.setComparatorPolarity(0);
}
