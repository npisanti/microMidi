
//MidiPiezos.ino
//Nicola Pisanti, GPLv3 License, 2016


#include "MIDIUSB.h"

// PIEZO SENSOR MIDI BOARD

// globals --------------------------------------------------------------------
#define MIDI_CHANNEL 1

// decomment for console output instead of midi
//#define CONSOLE_DEBUG

#ifdef CONSOLE_DEBUG
    // select the debug pin
    #define DEBUG_PIN A5
    // plot the signal
    #define DEBUG_PLOT
    // plot the signal without filtering
    //#define DEBUG_PLOT_RAW
    #define TEST_GATE_LO 40
    #define TEST_GATE_HI 600
    #define TEST_ENV_LO 10
    #define TEST_ENV_HI 240
#endif

// all the inputs must have a sensor connected, otherwise you will get noisy outputs
#ifdef CONSOLE_DEBUG
#define INPUTS 1
#else 
    #define INPUTS 6
#endif

// this has to be a negative number
#define HYSTERESYS -30


// sensor struct ...-----------------------------------------------------------
struct PiezoInput {
    int pin;
    bool gateOut;
    int  gateThresholdMin;
    int  gateThresholdMax;
    int  gateCounter;
    bool gateDirection;
    int  gateNote;
    bool envOut;
    int  envThresholdMin;
    int  envThresholdMax;
    float envAtkCoeff;
    float envRelCoeff;
    float envYn;
    int  envCC;
    int envLastOutput;
    int lastValue;
};

// ----------------------------------------------------------------------------
PiezoInput sensors[INPUTS];

void setup() {
    
    // default values
    for (int i = 0; i < INPUTS; ++i) {
        sensors[i].pin = A0 + i;
        sensors[i].lastValue = 0;
        sensors[i].gateCounter = 1;
        sensors[i].gateDirection = false;
        sensors[i].envYn = 0.0f;
        sensors[i].envLastOutput = 0;
        
        sensors[i].gateOut = true;
        sensors[i].gateThresholdMin = 10;
        sensors[i].gateThresholdMax = 100;
        sensors[i].gateNote = 60 + i;
        
        sensors[i].envOut = true;
        sensors[i].envThresholdMin = 20;
        sensors[i].envThresholdMax = 400;
        sensors[i].envAtkCoeff = 0.8f;
        sensors[i].envRelCoeff = 0.98f;
        sensors[i].envCC = i + 1;
    }
    
#ifndef CONSOLE_DEBUG

    // set up custom values here

    // HI HAT
    sensors[0].gateThresholdMin = 5;
    sensors[0].gateThresholdMax = 800;
    sensors[0].envThresholdMin = 10;
    sensors[0].envThresholdMax = 200;
    sensors[0].envRelCoeff = 0.9f;

    // KICK
    sensors[1].gateThresholdMin = 250;
    sensors[1].gateThresholdMax = 1024;
    sensors[1].envThresholdMin = 350;
    sensors[1].envThresholdMax = 750;    

    // SNARE
    sensors[2].gateThresholdMin = 80;
    sensors[2].gateThresholdMax = 800;
    sensors[2].envThresholdMin = 80;
    sensors[2].envThresholdMax = 600;   

    // TOM 1
    sensors[3].gateThresholdMin = 200;
    sensors[3].gateThresholdMax = 800;
    sensors[3].envThresholdMin = 120;
    sensors[3].envThresholdMax = 550;    

    // TOM 2
    sensors[4].gateThresholdMin = 200;
    sensors[4].gateThresholdMax = 900;
    sensors[4].envThresholdMin = 200;
    sensors[4].envThresholdMax = 650;

    // RIDE
    sensors[5].gateThresholdMin = 40;
    sensors[5].gateThresholdMax = 600;
    sensors[5].envThresholdMin = 10;
    sensors[5].envThresholdMax = 240;
    sensors[5].envRelCoeff = 0.99f;

#else
    // TEST VALUES
    sensors[0].pin = DEBUG_PIN;
    sensors[0].gateThresholdMin = TEST_GATE_LO;
    sensors[0].gateThresholdMax = TEST_GATE_HI;
    sensors[0].envThresholdMin = TEST_ENV_LO;
    sensors[0].envThresholdMax = TEST_ENV_HI;
    sensors[0].envRelCoeff = 0.99f;
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
        int sensorValue = analogRead( sensors[i].pin );
        
        if ( sensors[i].gateOut) { // TRIGGER ROUTINE --------------------------------------------------
        
            if ( sensors[i].gateCounter > 0 ) {
                
                bool directionNow = (sensorValue > sensors[i].lastValue );
                
                if ( sensors[i].gateDirection && !directionNow && (sensorValue > sensors[i].gateThresholdMin) ) { // got peak value
                    
                    sensors[i].gateCounter = HYSTERESYS; // reset counter for debouncing
                    
                    // calculate and send midi
                    int velo = constrain( sensorValue, sensors[i].gateThresholdMin, sensors[i].gateThresholdMax );
                    velo = map (velo, sensors[i].gateThresholdMin, sensors[i].gateThresholdMax, 1, 127 );
                    
    #ifdef CONSOLE_DEBUG
    #ifndef DEBUG_PLOT
                    Serial.print("sending note on, note ");
                    Serial.print(sensors[i].gateNote);
                    Serial.print(", velo ");
                    Serial.print(velo);
                    Serial.print(" from pin ");
                    Serial.println( sensors[i].pin );
    #endif
    #else
                    noteOn( MIDI_CHANNEL, sensors[i].gateNote, velo );
    #endif
                }
                
                sensors[i].gateDirection = directionNow;
            }else{
                sensors[i].gateCounter++;
            }
          
        } // end of trigger routine -------------------------------------------------------------
        
        
        if ( sensors[i].envOut ) { // CC ROUTINE --------------------------------------------------
            
            // ENVELOPE FOLLOWER CODE
            float xn = sensorValue;
            if (xn > sensors[i].envYn ) {
                sensors[i].envYn = sensors[i].envAtkCoeff * (sensors[i].envYn - xn) + xn;
            } else {
                sensors[i].envYn = sensors[i].envRelCoeff * (sensors[i].envYn - xn) + xn;
            }
            
            // MAP VALUES TO RANGE
            int out = constrain( sensors[i].envYn, sensors[i].envThresholdMin, sensors[i].envThresholdMax );
            out = map (out, sensors[i].envThresholdMin, sensors[i].envThresholdMax, 0, 127 );
            
            if (out != sensors[i].envLastOutput ) {
    #ifdef CONSOLE_DEBUG
    #ifndef DEBUG_PLOT
            //Serial.print("sending CC, control ");
            //Serial.print(sensors[i].envCC);
            //Serial.print(", value ");
            //Serial.print(out);
            //Serial.print(" from pin ");
            //Serial.println( sensors[i].pin );
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


