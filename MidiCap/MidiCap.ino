#include <CapacitiveSensor.h>
#include <MIDIUSB.h>

//MidiCap.ino
//Nicola Pisanti, GPLv3 License, 2016

// CAPACITIVE SENSORS TO MIDI

// GLOBAL VARS -----------------------------------------------------------------
#define MIDI_CHANNEL 1

// decomment for console output instead of midi
//#define CONSOLE_DEBUG

#ifdef CONSOLE_DEBUG
    // select the debug pin
    #define DEBUG_PIN 2
    
    // plot the signal
    //#define DEBUG_PLOT
    // plot the signal without filtering
    #define DEBUG_PLOT_RAW
#endif


#ifdef CONSOLE_DEBUG
#define INPUTS 1
#else 
    #define INPUTS 1
#endif

#define SENDPIN 12
#define SAMPLES 10
#define DEFAULT_THRESHOLD_MIN 1000
#define DEFAULT_THRESHOLD_MAX 6000
#define DEFAULT_THRESHOLD_TOUCH 10000


// globals --------------------------------------------------------------------

// sensor struct ...-----------------------------------------------------------
struct CapInput {

    CapacitiveSensor* cap;
    
    int pin;
    bool gateOut;    
    bool envOut;

    int  proximityThresholdMax;
    int  proximityThresholdMin;

    int  touchThreshold;
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

CapInput sensors[INPUTS];

void setup() {
    
    // default values
    for (int i = 0; i < INPUTS; ++i) {
        
        sensors[i].pin = 2 + i;
        sensors[i].lastValue = 0;
        sensors[i].envYn = 0.0f;
        sensors[i].envLastOutput = 0;
        sensors[i].gateActive = false;
                
        sensors[i].gateOut = true;
        sensors[i].touchThreshold = DEFAULT_THRESHOLD_TOUCH;
        
        sensors[i].envOut = true;
        
        sensors[i].proximityThresholdMin = DEFAULT_THRESHOLD_MIN;
        sensors[i].proximityThresholdMax = DEFAULT_THRESHOLD_MAX;
        sensors[i].gateNote = 60 + i;
 
        sensors[i].envAlpha = 0.9f;
        sensors[i].envCC = i + 1;
    }
    
#ifndef CONSOLE_DEBUG
    // set up custom values here
    // for example 
    //sensors[0].pin = 4;
    //sensors[0].proximityValueMin     = 20;
    //sensors[1].proximityThresholdMin = 0;
    //sensors[1].proximityThresholdMax = 10000;
    //sensors[2].gateNote = 60 + i;
    //sensors[2].envRelCoeff = 0.99f;
#else
    // TEST VALUES
    //sensors[0].pin = DEBUG_PIN;
    //sensors[0].proximityThresholdMin = 0;
    //sensors[0].proximityThresholdMax = 10000;
    //sensors[0].touchThreshold = 16000;    
    //sensors[0].gateNote = 60 + i;
    //sensors[0].envRelCoeff = 0.99f;
#endif

    
    // set up capacitive sensors
    for (int i = 0; i < INPUTS; ++i) {
        sensors[i].cap = new CapacitiveSensor(SENDPIN, sensors[i].pin);
        
        // decomment to turn off autocalibrate on all channell
        //sensors[i].cap->set_CS_AutocaL_Millis(0xFFFFFFFF); 
    }
        
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
        
        int sensorValue = abs(sensors[i].cap->capacitiveSensor( SAMPLES ));
        
        if ( sensors[i].gateOut) { // TRIGGER ROUTINE --------------------------------------------------
           
            if ( sensors[i].gateActive ) {
                    
                    if(sensorValue < sensors[i].touchThreshold){
                      
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

                    if(sensorValue > sensors[i].touchThreshold){
                        
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
            int out = constrain( sensors[i].envYn, sensors[i].proximityThresholdMin, sensors[i].proximityThresholdMax ); 
            out = map (out , sensors[i].proximityThresholdMin, 
                                   sensors[i].proximityThresholdMax, 
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


