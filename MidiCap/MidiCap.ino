#include <CapacitiveSensor.h>
#include <MIDIUSB.h>

//MidiCap.ino
//Nicola Pisanti, GPLv3 License, 2016-2017

// CAPACITIVE SENSORS TO MIDI
// TOUCH ONLY

// GLOBAL VARS -----------------------------------------------------------------
#define MIDI_CHANNEL 1
#define INPUTS 2
#define DEFAULT_THRESHOLD_ON 3000
#define DEFAULT_THRESHOLD_OFF 1500
#define SENDPIN 12
#define SAMPLES 10

// for setting custom value to each different sensors
// go inside the setup to the CUSTOM VALUES section

// decomment for console output instead of midi
#define CONSOLE_DEBUG

#ifdef CONSOLE_DEBUG    
    // plot the signal, send messages to console if decommented
    #define DEBUG_PLOT
#endif
// end globals ----------------------------------------------------------------

// sensor struct ...-----------------------------------------------------------
struct CapInput {

    CapacitiveSensor* cap;
    
    int pin;   

    int  thresholdOff;
    int  thresholdOn;

    int  gateNote; // note for this sensor on touch   
    bool gateActive;

};

// ----------------------------------------------------------------------------

CapInput sensors[INPUTS];

void setup() {
    
    // default values
    for (int i = 0; i < INPUTS; ++i) {
        sensors[i].pin = 2 + i;
        sensors[i].gateActive = false;
        sensors[i].thresholdOn = DEFAULT_THRESHOLD_ON;
        sensors[i].thresholdOff = DEFAULT_THRESHOLD_OFF;
        sensors[i].gateNote = 60 + i;
    }

#ifdef DEBUG_PLOT
    //sensors[0].pin = 2; //change test pin only on testing
#endif

    // set up custom values for the sketch here ------------CUSTOM VALUES---------------
    // for example 
    //sensors[0].pin = 4;
    //sensors[1].thresholdOn = 0;
    //sensors[1].thresholdOff = 10000;
    //sensors[2].gateNote = 60 + i;
    // --------------------------------------------------------------------------------

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
        
        int sensorValue = sensors[i].cap->capacitiveSensor( SAMPLES );
        if(sensorValue<0) sensorValue = -sensorValue;
        
#ifdef DEBUG_PLOT
        if(i==INPUTS-1){
          Serial.println(sensorValue);
        }else{
          Serial.print(sensorValue);
          Serial.print(",");
        }
#endif

        if ( sensors[i].gateActive ) {
                
                if(sensorValue < sensors[i].thresholdOff){
                  
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
                
        } else {

                if(sensorValue > sensors[i].thresholdOn){
                    
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
    }   
    
#ifndef CONSOLE_DEBUG
    MidiUSB.flush(); // send midi
#endif
    delay(5);        // delay in between reads for stability

}


