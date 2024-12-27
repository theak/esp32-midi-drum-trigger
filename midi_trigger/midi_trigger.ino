#include <Arduino.h>
#include <BLEMidi.h>

#include <unordered_set>

std::unordered_set<int> heldNotes; // Holds currently pressed notes

#define BUTTON_PIN 13
#define LED_PIN LED_BUILTIN
#define SNARE_PIN 36
#define KICK_PIN 39

#define KICK_NOTE 36
#define SNARE_NOTE 38

const int notes[] = {SNARE_NOTE, KICK_NOTE};
const int pins[] = {SNARE_PIN, KICK_PIN};

const int min_threshold = 50;
const int min_consecutive = 4;
const int ms_to_listen = 10;
const int ms_to_wait = 150;

int num_consecutive = 0;
int trigger_time = 0;
int last_sent = 0;
int intensity = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing bluetooth");
  BLEMidiServer.begin("Esp32 MIDI device");
  Serial.println("Waiting for connections...");
  //BLEMidiServer.enableDebugging();  // Uncomment for debug output from the library
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  int val = analogRead(SNARE_PIN);
  
  if (val > min_threshold) {
    Serial.println(num_consecutive);
    if (num_consecutive == 0) trigger_time = millis();
    num_consecutive += 1;
    if (val > intensity) intensity = val;
    if (num_consecutive >= min_consecutive
        && (millis() - last_sent) >= ms_to_wait) {
          digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Toggle LED when sending note
          last_sent = millis();
          Serial.println("sending note");
          Serial.println(intensity);
          if(BLEMidiServer.isConnected()) {
            sendNote(SNARE_NOTE, 127, 0);
          }
          intensity = 0;
        }
  } else {
    num_consecutive = 0;
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
