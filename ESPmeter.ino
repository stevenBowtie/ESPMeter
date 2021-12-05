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
U8G2_SSD1306_128X64_NONAME_1_SW_I2C disp( U8G2_R0, DispSCL, DispSDA );

//MQTT
#include <WiFiClient.h>
#include "lib/pubsubclient/src/PubSubClient.h"
#include "lib/pubsubclient/src/PubSubClient.cpp"

const char* ap_ssid = "ESPMeter";
const char* ap_pass = "nopassword";
//const char* sta_ssid = "CMIWIFI";
//const char* sta_pass = "supersecretpassword";
#include "password.h"
IPAddress apIP;
IPAddress staIP;

#define RANGE_THRESHOLD 10
#define HBpin  5
#define VBATT_IN 34
bool HB = 0;
float vbatt = 0;
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
float ads_scaling[] = { 111.44, 0, 0 };
int8_t reading_sign[] = { 1, 1, 1, 1 };
const uint8_t muxBits[] = { ADS1115_MUX_P0_N3, ADS1115_MUX_P1_N3, ADS1115_MUX_P2_N3 };

unsigned long cycle_count = 0;

//MQTT
WiFiClient wifi_client;
PubSubClient mqtt_client( wifi_client );
IPAddress mqtt_server( 10,41,2,18 ); //Grafanflux
char toString[16] = { 0 };
unsigned long lastPrint = 0;

#define BATT_BUTTON 15
#define RELAY_PIN 2
int buttonCount = 0;
bool button = 0;
bool batt_discharge = 0;
float voltage = 0;
float voltage_threshold = 11.1;

void mqtt_publish_float( char topic[], float value ){
  dtostrf( value, 10, 2, toString );
  if( !mqtt_client.connected() ){ 
    mqtt_client.connect( baseMacChr );
  }
  char topic_buf[50] = {0};
  sprintf( topic_buf, "ESPMeter/%s/%s", baseMacChr, topic );
  mqtt_client.publish( topic_buf, toString );
}

MeterConfig mc;

void setup() {
  pinMode( HBpin, OUTPUT );
  pinMode( BATT_BUTTON, INPUT_PULLUP );
  pinMode( VBATT_IN, INPUT );
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

  //MQTT
  mqtt_client.setServer( mqtt_server, 1883 );
  mqtt_client.connect( baseMacChr );
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
      Serial.print( "VBatt=" );
      Serial.println( vbatt );
    }
  readBatt(); 
  updateDisplay();
  mqtt_loop();
}

void battery_test(){
  buttonCount = digitalRead( BATT_BUTTON ) ? buttonCount++ : buttonCount-- ;
  buttonCount = (buttonCount < 0) ? buttonCount : 0;
  buttonCount = (buttonCount > 200) ? buttonCount : 200;
  button = (buttonCount > 100) ? 1 : 0;
  voltage = calc_reading( 0 );
  if( button && voltage > voltage_threshold ){
    batt_discharge = 1;
  }
  if( batt_discharge and voltage > voltage_threshold ){
    digitalWrite( RELAY_PIN, 1 );
  }
  else{
    digitalWrite( RELAY_PIN, 0 );
    batt_discharge = 0;
  }
}

float calc_reading( int chnl ){
  return reading_sign[chnl] * sqrt( ads_readings[chnl] ) * ads_scaling[chnl];
}

void mqtt_loop(){
  if( millis() - lastPrint > 200 ){
    mqtt_publish_float( "v0", calc_reading(0) );
    mqtt_publish_float( "v1", calc_reading(1) );
    mqtt_publish_float( "v2", calc_reading(2) );
    mqtt_publish_float( "batt", vbatt );
    lastPrint = millis();
  }
}

void readBatt(){
  vbatt = ((vbatt*avg_factor) + (analogRead( VBATT_IN ) * 0.00175)) / (avg_factor+1);
}

void rdy_interrupt(){
  rdy_state = 1;
}

void ADCpoll(){       
  if(rdy_state || cycle_count>1000){
    rdy_state = 0;
    thisSample = adc.getMilliVolts(false);
    reading_sign[current_channel] = (thisSample > 0) ? 1 : -1 ;
    analogAvg = ads_readings[current_channel];
    ads_readings[current_channel] = 
      ((analogAvg*avg_factor)+pow(thisSample,2)) / (avg_factor+1);
    autorange( sqrt(ads_readings[current_channel]) );
    current_channel++;
    if( current_channel > 2 ){ current_channel = 0; }
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
