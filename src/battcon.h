class batt_con{

    public:
    enum state { CHARGE, DISCHARGE, DWELL, STOP } ;
    enum state current_state = STOP ;

    float batt_voltage ;                 // Current battery voltage
    float batt_current ;                 // Current amperage measurement
    float charge_voltage ;               // Charge termination voltage
    float charge_current ;               // Charge termination current
    unsigned long charge_timeout;        // Terminate charge if this timeout is reached
    float discharge_voltage ;            // Discharge termination voltage
    float discharge_current ;            // Discharge termination current
    unsigned long charge_begin ;         // Discharge time limit
    unsigned long discharge_time ;       // Discharge time limit
    unsigned long dwell_time ;           // How long to wait in Dwell period
    unsigned long dwell_ended ;          // Time that dwell period ended
    unsigned long charge_ended ;         // Time that charge ended

    enum reason { NONE, CHARGE_CURRENT, CHARGE_TIMEOUT, 
                  DISCHARGE_VOLTAGE, DISCHARGE_CURRENT, DISCHARGE_TIME,
                  DWELL_MAX, DWELL_TIME } charge_end_reason, dwell_end_reason, discharge_end_reason ;

    #define charge_output_pin       1
    #define discharge_output_pin    2
    #define stop_pin                3

    void battcon_setup(){
        pinMode( charge_output_pin, OUTPUT );
        pinMode( discharge_output_pin, OUTPUT );
        pinMode( stop_pin, INPUT_PULLUP );
    }

    void battcon_loop(){
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
        if( digitalRead( stop_pin ) ){ current_state = STOP ; }
        batt_voltage = read_voltage() ;
        batt_current = read_current() ;
    }

    void charge(){
        digitalWrite( charge_output_pin, 1 ) ;
        if( batt_volage > charge_voltage && batt_current < charge_current ){ 
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
      if( millis() - charge_ended > dwell_max_time ){
        current_state = DISCHARGE ;
        dwell_ended = millis() ;
        dwell_end_reason = DWELL_MAX ;
      }
      
      //Only sample at the poll rate to allow some change between samples
      if( millis() - last_time > dwell_poll_rate ){     
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
    }

    float read_voltage(){

    }

    float read_current(){

    }
}
