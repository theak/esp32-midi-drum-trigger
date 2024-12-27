#include <Arduino.h>
#include <BLEMidi.h>
#include <unordered_set>

std::unordered_set<int> heldNotes; // Holds currently pressed notes

// Pin definitions
#define BUTTON_PIN 13
#define LED_PIN LED_BUILTIN
#define SNARE_PIN 36
#define KICK_PIN 39

// MIDI note definitions
#define KICK_NOTE 36
#define SNARE_NOTE 38

// Array of pins and corresponding notes
const int notes[] = {SNARE_NOTE, KICK_NOTE};
const int pins[] = {SNARE_PIN, KICK_PIN};
const int NUM_TRIGGERS = sizeof(pins) / sizeof(pins[0]);

// Trigger parameters
const int min_threshold = 75;
const int min_consecutive = 4;
const int ms_to_wait = 150;

// State tracking for each trigger
struct TriggerState {
    int num_consecutive = 0;
    int trigger_time = 0;
    int last_sent = 0;
    int intensity = 0;
};

TriggerState triggerStates[2]; // One state object for each trigger

void setup() {
    Serial.begin(9600);
    Serial.println("Initializing bluetooth");
    BLEMidiServer.begin("Esp32 MIDI device");
    Serial.println("Waiting for connections...");
    //BLEMidiServer.enableDebugging(); // Uncomment for debug output from the library
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    // Loop through all trigger pins
    for (int i = 0; i < NUM_TRIGGERS; i++) {
        int val = analogRead(pins[i]);
        TriggerState& state = triggerStates[i];
        
        if (val > min_threshold) {
            Serial.println(state.num_consecutive);
            if (state.num_consecutive == 0) {
                state.trigger_time = millis();
            }
            state.num_consecutive += 1;
            if (val > state.intensity) {
                state.intensity = val;
            }
            
            if (state.num_consecutive >= min_consecutive && 
                (millis() - state.last_sent) >= ms_to_wait) {
                
                state.last_sent = millis();
                
                Serial.println("sending note");
                Serial.println(notes[i]);
                
                if(BLEMidiServer.isConnected()) {
                    sendNote(notes[i], 127, 0);
                }
                digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Toggle LED when sending note
                state.intensity = 0;
            }
        } else {
            state.num_consecutive = 0;
        }
    }
}

void sendNote(int note, int velocity, int channel) {
    if (heldNotes.find(note) == heldNotes.end()) {
        BLEMidiServer.noteOff(channel, note, velocity);
        heldNotes.erase(note);
    }
    BLEMidiServer.noteOn(channel, note, velocity);
    heldNotes.insert(note);
}
