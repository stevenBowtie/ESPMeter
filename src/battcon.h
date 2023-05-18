#ifndef _BATTCON
#define _BATTCON
#include <Arduino.h>
#pragma message("Loaded battcon.h")

class BattCon{

  public:
  enum state { CHARGE, DWELL, DISCHARGE, STOP } ;
  enum state current_state = STOP ;

  float batt_voltage ;                 // Current battery voltage
  float batt_current ;                 // Current amperage measurement
  float charge_voltage ;               // Charge termination voltage
  float charge_current ;               // Charge termination current
  unsigned long charge_timeout;        // Terminate charge if this timeout is reached
  float discharge_voltage ;            // Discharge termination voltage
  float discharge_current ;            // Discharge termination current
	float last_reading;
  unsigned long charge_begin ;         // Discharge time limit
  unsigned long discharge_time ;       // Discharge time limit
  unsigned long dwell_time ;           // How long to wait in Dwell period
  unsigned long dwell_time_threshold ; // How long to wait in Dwell period
  unsigned long dwell_ended ;          // Time that dwell period ended
  unsigned long dwell_poll_rate = 30000 ; // How often to check the change in voltage
	unsigned long dwell_voltage_threshold  = 0.1; // Minimum voltage change to end dwell       
	unsigned long last_dwell_poll;       
	unsigned long lastChange;       
  unsigned long charge_ended ;         // Time that charge ended

    bool pressed_latch = 0;

    enum reason { NONE, CHARGE_CURRENT, CHARGE_TIMEOUT, 
                  DISCHARGE_VOLTAGE, DISCHARGE_CURRENT, DISCHARGE_TIME,
                  DWELL_MAX, DWELL_TIME } charge_end_reason, dwell_end_reason, discharge_end_reason ;

    #define charge_output_pin       19
    #define discharge_output_pin    23
    #define stop_pin                3
    #define PRESSED_THRESHOLD       200 

    void battcon_setup(){
        pinMode( charge_output_pin, OUTPUT );
        pinMode( discharge_output_pin, OUTPUT );
        pinMode( stop_pin, INPUT_PULLUP );
    }

    void battcon_loop( float v, float c ){
        switch ( current_state ){
            case STOP:
                break ;
            case CHARGE :
                charge() ;
                break ;
            case DISCHARGE :
                discharge() ;
                break ;
            case DWELL :
                dwell() ;
                break ;
        }
        if( button_debounced && !prev_button ){ 
          prev_button = 1;
          current_state = STOP ; 
        }
        batt_voltage = v ;
        batt_current = c ;
    }

    bool button_debounced(){
      if( digitalRead( stop_pin ) ){ pressed++; }
      else{ pressed--; }
      pressed = constrain( pressed, 0, 250 );
      if( pressed > PRESSED_THRESHOLD ){ pressed_latch = 1; }
      if( pressed == 0 ){ pressed_latch = 0; }
      return pressed_latch
    }

    void charge(){
        digitalWrite( charge_output_pin, 1 ) ;
        if( button_debounced() || (batt_voltage > charge_voltage && batt_current < charge_current) ){ 
            current_state = DWELL ; 
            charge_ended = millis() ;
            charge_end_reason = CHARGE_CURRENT;
            digitalWrite( charge_output_pin, 0 ) ;
        }
        if( millis()-charge_begin > charge_timeout ){
            current_state = STOP ;
            charge_ended = millis() ;
            charge_end_reason = CHARGE_TIMEOUT ;
            digitalWrite( charge_output_pin, 0 ) ;
        }
    }

    void dwell(){
      //Wait until battery is stable or timeout
      if( millis() - charge_ended > dwell_time ){
        current_state = DISCHARGE ;
        dwell_ended = millis() ;
        dwell_end_reason = DWELL_MAX ;
      }
      
      //Only sample at the poll rate to allow some change between samples
      if( millis() - last_dwell_poll > dwell_poll_rate ){     
        if( batt_voltage - last_reading > dwell_voltage_threshold ){
          lastChange = millis() ;
          last_reading = batt_voltage ;
        }
      }

      if( millis() - lastChange > dwell_time_threshold ){
        current_state = DISCHARGE ;
        dwell_ended = millis() ;
        dwell_end_reason = DWELL_TIME ;
      }
    }

    void discharge(){
      //Discharge for time or voltage threshold
      digitalWrite( discharge_pin, 1 );
      if( batt_voltage < discharge_voltage ){
        digitalWrite( discharge_pin, 0 );
        current_state = STOP;
        discharge_end_reason = DISCHARGE_VOLTAGE;
      }
    }

    float read_voltage(){
		float voltage;
		return voltage;
    }

    float read_current(){
		float current;
		return current;
    }
};
#endif
