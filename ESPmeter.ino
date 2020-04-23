#include <Arduino.h>
#include "serverConfig.h"
#include "config.h"
#include <ESPmDNS.h>
#include <Wire.h>

//ADS1115
#include "lib\I2Cdev.h"
#include "lib\I2Cdev.cpp"
#include "lib\ADS1115.h"
#include "lib\ADS1115.cpp"
#define SDApin  14
#define SCLpin  27

//Display
#include <U8g2lib.h>
#define DispSDA 21
#define DispSCL 22
TwoWire DispWire = TwoWire(1);
//U8G2_ST75320_JLX320240_1_SW_I2C disp( U8G2_R0, DispSCL, DispSDA );
U8G2_SSD1306_128X64_NONAME_1_SW_I2C disp( U8G2_R0, DispSCL, DispSDA );
 

const char* ap_ssid = "ESPMeter";
const char* ap_pass = "nopassword";
//const char* sta_ssid = "CMIWIFI";
//const char* sta_pass = "supersecretpassword";
#include "password.h"
IPAddress apIP;
IPAddress staIP;

#define RANGE_THRESHOLD 10
#define HBpin  5
bool HB = 0;
int avg_factor = 10;
unsigned long analogAvg = 0;
unsigned long pollTime = 0;
unsigned long lastUpdate = 0;
long chan0 = 0;

byte flag = 1;

ADS1115 adc(ADS1115_DEFAULT_ADDRESS);
const int alertReadyPin = 26;
bool rdy_state = 1;
float reading = 0;
float thisSample;
const float rangeMax[] = { 6144, 4096, 1024, 512, 256 } ;

uint8_t current_channel = 0;
float ads_readings[] = { 0, 0, 0, 0 };
const uint8_t muxBits[] = { ADS1115_MUX_P0_N1, ADS1115_MUX_P1_NG, ADS1115_MUX_P2_NG, ADS1115_MUX_P3_NG };

unsigned long cycle_count = 0;

MeterConfig mc;

void setup() {
  pinMode( HBpin, OUTPUT );
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(sta_ssid,sta_pass);
  WiFi.softAP(ap_ssid, ap_pass);
  apIP = WiFi.softAPIP();
  staIP = WiFi.localIP();
  Serial.print("AP: ");
  Serial.println(apIP);
  Serial.print("STA: ");
  Serial.println(staIP);
  mc.load_config();   
  server.begin();
  print_spiffs();
  init_server_callbacks();

  //Display
  //DispWire.begin( DispSDA, DispSCL );
  disp.begin();

  //ADC config
  resetI2C();
  delay(1);
  Wire.begin( SDApin, SCLpin );
  pinMode(alertReadyPin,INPUT_PULLUP);
  attachInterrupt( alertReadyPin, rdy_interrupt, RISING );
  adc.initialize();
  setADCconfig(); 
  #ifdef ADS1115_SERIAL_DEBUG
  adc.showConfigRegister();
  Serial.print("HighThreshold="); Serial.println(adc.getHighThreshold());
  Serial.print("LowThreshold="); Serial.println(adc.getLowThreshold());
  #endif
  adc.triggerConversion();

  //mDNS
  getMacName();
  MDNS.begin(baseMacChr);
  MDNS.addService("http", "tcp", 80);  
}

void loop() {
  chan0 =sqrt(ads_readings[0]);
  pollTime = micros();
  ADCpoll(); 
  analogAvg=((analogAvg*avg_factor)+(micros()-pollTime))/(avg_factor+1);
  if( millis() - lastUpdate > 100 ){
    lastUpdate = millis();
      flag = 0;
      digitalWrite( HBpin, HB );
      //Serial.println( adc.sendConfig() );
      HB = !HB;
      Serial.println( sqrt( ads_readings[0] ) );
      Serial.println( rangeMax[ adc.getGain() ] );
    }
  updateDisplay();
}

void rdy_interrupt(){
  rdy_state = 1;
}

void ADCpoll(){       
  if(rdy_state || cycle_count>1000){
    rdy_state = 0;
    thisSample = adc.getMilliVolts(false);
    int8_t sampleSign = (thisSample > 0) ? 1 : -1 ;
    analogAvg = ads_readings[current_channel];
    ads_readings[current_channel] = 
      ((analogAvg*avg_factor)+pow(thisSample,2)) / (avg_factor+1);
    autorange( sqrt(ads_readings[current_channel]) );
    adc.setMultiplexer( muxBits[current_channel] );
    adc.triggerConversion();
  //Serial.println(current_channel);
/*
    for(int i=0; i<4; i++){
      Serial.print(sqrt(ads_readings[i]));
      Serial.print(",");
    } 
  Serial.println(cycle_count);
*/
  cycle_count=0;
  }
  cycle_count++;
}

void setADCconfig(){
  adc.setMode(ADS1115_MODE_SINGLESHOT);
  adc.setRate(ADS1115_RATE_860);
  adc.setGain(ADS1115_PGA_6P144);
  adc.setComparatorQueueMode(ADS1115_COMP_QUE_ASSERT1);
  adc.setComparatorPolarity(1);
  adc.setConversionReadyPinMode();
  adc.sendConfig();
}

void resetI2C(){
  Wire.beginTransmission(0);
  Wire.write(0x06);
  Wire.endTransmission();
}

void autorange(float ar_reading){
  uint8_t nowGain = adc.getGain();
  if( abs(ar_reading) >= rangeMax[ nowGain ] - RANGE_THRESHOLD ){
    adc.setGain( max( 0, min(4, nowGain-1) ) );
  }
  else{
    nowGain = max( 0, min(4, nowGain+1) );
    if( abs(ar_reading) <= rangeMax[ nowGain ] ){
    adc.setGain( max( 0, min(4, nowGain+1) ) );
    }
  }
}

void updateDisplay(){
  disp.firstPage();
  do {
    disp.setFont(u8g2_font_ncenB14_tr);
    disp.drawStr(0,24,"ESPmeter");
  } while ( disp.nextPage() );
}
