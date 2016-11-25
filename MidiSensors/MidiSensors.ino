
#include <MIDIUSB.h>

//MidiSensors.ino
//Nicola Pisanti, GPLv3 License, 2016

// SENSORS TO MIDI

// GLOBAL VARS --------------------------------------------------------------------
#define MIDI_CHANNEL 1
#define DEFAULT_THRESHOLD_MIN 0
#define DEFAULT_THRESHOLD_MAX 1024
#define DEFAULT_THRESHOLD_GATE 512
#define INPUTS 1

// for setting custom value to each different sensors
// go inside the setup to the CUSTOM VALUES section

// decomment for console output instead of midi
//#define CONSOLE_DEBUG

// custom flags for console outputs
#ifdef CONSOLE_DEBUG
    // select the debug pin
    #define DEBUG_PIN A0

    // plot the signal
    //#define DEBUG_PLOT

    // plot the signal without filtering
    #define DEBUG_PLOT_RAW
#endif
// globals end --------------------------------------------------------------------


#ifdef CONSOLE_DEBUG
    #define INPUTS 1
#endif

// sensor struct ...-----------------------------------------------------------
struct SensorInput {

    int pin;
    bool gateOut;    
    bool envOut;

    int  sensorThresholdMax;
    int  sensorThresholdMin;

    int  gateThreshold;
    int  gateNote; // note for this sensor on touch   
    bool gateActive;

    int  envThresholdMin;
    int  envThresholdMax;
    float envAlpha;
    float envYn;
    int   envCC;
    int   envLastOutput;
    int   lastValue;
};

// ----------------------------------------------------------------------------

SensorInput sensors[INPUTS];

void setup() {
    
    // default values
    for (int i = 0; i < INPUTS; ++i) {
        
        sensors[i].pin = A0 + i;
        sensors[i].lastValue = 0;
        sensors[i].envYn = 0.0f;
        sensors[i].envLastOutput = 0;
        sensors[i].gateActive = false;
                
        sensors[i].gateOut = false;  // by default we just send CCs
        sensors[i].gateThreshold = DEFAULT_THRESHOLD_GATE;
        
        sensors[i].envOut = true;
        
        sensors[i].sensorThresholdMin = DEFAULT_THRESHOLD_MIN;
        sensors[i].sensorThresholdMax = DEFAULT_THRESHOLD_MAX;
        sensors[i].gateNote = 60 + i;
 
        sensors[i].envAlpha = 0.9f;
        sensors[i].envCC = i + 1;
    }

    
#ifdef CONSOLE_DEBUG
    // set up your custom test values here
    //sensors[0].pin = DEBUG_PIN;
    //sensors[0].sensorThresholdMin = 0;
    //sensors[0].sensorThresholdMax = 10000;
    //sensors[0].gateThreshold = 16000;    
    //sensors[0].gateNote = 60 + i;
    //sensors[0].envRelCoeff = 0.99f;
#else
    // set up custom values for the sketch here ------------CUSTOM VALUES---------------
    // for example 
    //sensors[0].pin = A4;
    //sensors[1].sensorThresholdMin = 0;
    //sensors[1].sensorThresholdMax = 10000;
    //sensors[2].gateNote = 60 + i;
    //sensors[2].envRelCoeff = 0.99f;
    // --------------------------------------------------------------------------------
#endif

        
#ifdef CONSOLE_DEBUG
    Serial.begin(57600);
#endif
}

// helper methods -------------------------------------------------------------
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}


// loop -----------------------------------------------------------------------
void loop() {

    for ( int i = 0; i < INPUTS; ++i ) {
        
        int sensorValue = analogRead(sensors[i].pin);
        
        if ( sensors[i].gateOut) { // TRIGGER ROUTINE --------------------------------------------------
           
            if ( sensors[i].gateActive ) {
                    
                    if(sensorValue < sensors[i].gateThreshold){
                      
                        sensors[i].gateActive = false;              
                        
    #ifdef CONSOLE_DEBUG
    #ifndef DEBUG_PLOT
                        Serial.print("sending note off, note ");
                        Serial.print(sensors[i].gateNote);
                        Serial.print(" from pin ");
                        Serial.println( sensors[i].pin );
    #endif
    #else
                        noteOff( MIDI_CHANNEL, sensors[i].gateNote, 127 );
    #endif
                        
                    }
                    
            }else{

                    if(sensorValue > sensors[i].gateThreshold){
                        
                        sensors[i].gateActive = true;
                                            
    #ifdef CONSOLE_DEBUG
    #ifndef DEBUG_PLOT
                        Serial.print("sending note on, note ");
                        Serial.print(sensors[i].gateNote);
                        Serial.print(" from pin ");
                        Serial.println( sensors[i].pin );
    #endif
    #else
                        noteOn( MIDI_CHANNEL, sensors[i].gateNote, 127 );
    #endif
                    }
            
            }
          
        } // end of trigger routine -------------------------------------------------------------
        
        
        if ( sensors[i].envOut ) { // CC ROUTINE --------------------------------------------------
            
            // LP FILTER CODE
            float xn = sensorValue;
            sensors[i].envYn = sensors[i].envAlpha * (sensors[i].envYn - xn) + xn;

            // MAPPING VALUES
            int out = constrain( sensors[i].envYn, sensors[i].sensorThresholdMin, sensors[i].sensorThresholdMax ); 
            out = map (out , sensors[i].sensorThresholdMin, 
                                   sensors[i].sensorThresholdMax, 
                                   0, 127 );
            
            if (out != sensors[i].envLastOutput ) {
#ifdef CONSOLE_DEBUG
#ifndef DEBUG_PLOT
                Serial.print("sending CC, control ");
                Serial.print(sensors[i].envCC);
                Serial.print(", value ");
                Serial.print(out);
                Serial.print(" from pin ");
                Serial.println( sensors[i].pin );
#endif
#else
                controlChange( MIDI_CHANNEL, sensors[i].envCC, out );
#endif
          }
 
          sensors[i].envLastOutput = out;
        
        } // end of cc routine ------------------------------------------------------------------
    
        sensors[i].lastValue = sensorValue;
    
#ifdef DEBUG_PLOT
#ifdef DEBUG_PLOT_RAW
        Serial.println(sensorValue);
        //Serial.println(sensors[i].envYn);
#else
        Serial.println(sensors[0].envLastOutput);
#endif
#endif
    
#ifndef CONSOLE_DEBUG
        MidiUSB.flush(); // send midi
#endif
    }   
    
    delay(5);        // delay in between reads for stability
}


