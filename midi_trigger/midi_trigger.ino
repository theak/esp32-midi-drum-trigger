#include <Arduino.h>
#include <BLEMidi.h>

#include <unordered_set>

std::unordered_set<int> heldNotes; // Holds currently pressed notes

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing bluetooth");
  BLEMidiServer.begin("Esp32 MIDI device");
  Serial.println("Waiting for connections...");
  //BLEMidiServer.enableDebugging();  // Uncomment for debug output from the library
}

void loop() {
  if(BLEMidiServer.isConnected()) {
    sendNote(69, 127, 0);
    delay(3000);
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
