#include <Arduino.h>
#include <BLEMidi.h>
#include <unordered_set>

std::unordered_set<int> heldNotes;

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

// Enhanced trigger parameters
const int TRIGGER_THRESHOLD[] = {250, 750};  // Different thresholds for snare and kick
const int NOISE_THRESHOLD = 50;              // Ignore readings below this
const int SLOPE_WINDOW = 3;                  // Window size for slope detection
const int RETRIGGER_TIME = 100;              // Minimum ms between triggers
const float VELOCITY_SCALING = 0.7f;         // Scale factor for velocity sensitivity
const int SLOPE_THRESHOLD = 30;              // Minimum increase between consecutive readings
const float CROSSTALK_RATIO = 5.0f;         // Required ratio between direct hit and sympathetic vibration
const int CROSSTALK_WINDOW = 15;             // ms to check for cross-talk

// State tracking for each trigger
struct TriggerState {
    int lastReadings[SLOPE_WINDOW];
    unsigned long lastTriggerTime = 0;
    int peakValue = 0;
    bool isTriggering = false;
    int recentMax = 0;                      // Track recent maximum for cross-talk detection
    unsigned long recentMaxTime = 0;        // When we last saw the maximum
};

TriggerState triggerStates[NUM_TRIGGERS];

void setup() {
    Serial.begin(9600);
    Serial.println("Initializing bluetooth");
    BLEMidiServer.begin("Esp32 MIDI device");
    Serial.println("Waiting for connections...");
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    
    // Initialize reading arrays
    for (int i = 0; i < NUM_TRIGGERS; i++) {
        for (int j = 0; j < SLOPE_WINDOW; j++) {
            triggerStates[i].lastReadings[j] = 0;
        }
    }
}

// Check for rapid rise in signal (positive slope)
bool detectRisingEdge(TriggerState& state, int currentValue) {
    // Shift readings
    for (int i = SLOPE_WINDOW - 1; i > 0; i--) {
        state.lastReadings[i] = state.lastReadings[i-1];
    }
    state.lastReadings[0] = currentValue;
    
    // Check for consistent rise
    for (int i = 0; i < SLOPE_WINDOW - 1; i++) {
        if (state.lastReadings[i] <= state.lastReadings[i+1] || 
            (state.lastReadings[i] - state.lastReadings[i+1]) < SLOPE_THRESHOLD) {
            return false;
        }
    }
    
    return true;
}

// Calculate MIDI velocity based on initial slope
int calculateVelocity(TriggerState& state) {
    float slopeMagnitude = state.lastReadings[0] - state.lastReadings[SLOPE_WINDOW-1];
    float velocity = slopeMagnitude * VELOCITY_SCALING;
    return constrain(map(velocity, 0, 1023, 0, 127), 1, 127);
}

// Check if this might be cross-talk from another trigger
bool isCrossTalk(int triggerIndex, int currentValue, unsigned long currentTime) {
    // Check all other triggers
    for (int i = 0; i < NUM_TRIGGERS; i++) {
        if (i == triggerIndex) continue;
        
        TriggerState& otherState = triggerStates[i];
        
        // If another trigger recently had a larger signal, this might be cross-talk
        if ((currentTime - otherState.recentMaxTime) < CROSSTALK_WINDOW &&
            otherState.recentMax > (currentValue * CROSSTALK_RATIO)) {
            return true;
        }
    }
    return false;
}

void loop() {
    unsigned long currentTime = millis();
    
    // First pass: update recent maximums for all triggers
    for (int i = 0; i < NUM_TRIGGERS; i++) {
        TriggerState& state = triggerStates[i];
        int currentValue = analogRead(pins[i]);
        
        // Update recent maximum if we see a larger value
        if (currentValue > state.recentMax || 
            (currentTime - state.recentMaxTime) > CROSSTALK_WINDOW) {
            state.recentMax = currentValue;
            state.recentMaxTime = currentTime;
        }
    }
    
    // Second pass: process triggers
    for (int i = 0; i < NUM_TRIGGERS; i++) {
        TriggerState& state = triggerStates[i];
        int currentValue = analogRead(pins[i]);
        
        // Ignore noise
        if (currentValue < NOISE_THRESHOLD) {
            state.isTriggering = false;
            continue;
        }
        
        // Check if enough time has passed since last trigger
        if (currentTime - state.lastTriggerTime < RETRIGGER_TIME) {
            continue;
        }
        
        // Rising edge detection with cross-talk check
        if (!state.isTriggering && 
            currentValue > TRIGGER_THRESHOLD[i] && 
            detectRisingEdge(state, currentValue) &&
            !isCrossTalk(i, currentValue, currentTime)) {
            
            if (BLEMidiServer.isConnected()) {
                int velocity = calculateVelocity(state);
                sendNote(notes[i], velocity, 0);
                state.lastTriggerTime = currentTime;
                state.isTriggering = true;
                
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
                
                Serial.print("Trigger ");
                Serial.print(i);
                Serial.print(" Value: ");
                Serial.print(currentValue);
                Serial.print(" Velocity: ");
                Serial.println(velocity);
            }
        }
        
        // Reset trigger state when signal drops
        if (currentValue < TRIGGER_THRESHOLD[i]) {
            state.isTriggering = false;
        }
    }
}

void sendNote(int note, int velocity, int channel) {
    if (heldNotes.find(note) == heldNotes.end()) {
        heldNotes.erase(note);
    }
    BLEMidiServer.noteOn(channel, note, velocity);
    BLEMidiServer.noteOff(channel, note, velocity);
    heldNotes.insert(note);
}