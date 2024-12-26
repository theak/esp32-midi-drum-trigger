#include <unordered_set>

#include <Bounce2.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

#define SerialMon Serial
#include <AppleMIDI.h>

#include "secrets.h"

unsigned long t0 = millis();
int8_t isConnected = 0;
std::unordered_set<int> heldNotes; // Holds currently pressed notes

APPLEMIDI_CREATE_DEFAULTSESSION_INSTANCE();

void setup() {
  // Connect to WiFi
  WiFi.begin(ssid, password);

  Serial.begin(9600);
  while (!Serial) ;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Establishing connection to WiFi..");
  }
  Serial.println("Connected to network");

  Serial.println("OK, now make sure you an rtpMIDI session that is Enabled");
  Serial.println("Add device named Arduino with Host"), WiFi.localIP(), "Port", AppleMIDI.getPort(), "(Name", AppleMIDI.getName(), ")";
  Serial.println("Select and then press the Connect button");
  Serial.println("Then open a MIDI listener and monitor incoming notes");
  Serial.println("Listen to incoming MIDI commands");

  MIDI.begin();
  
  AppleMIDI.setHandleConnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc, const char* name) {
    isConnected++;
    Serial.println("Connected");
  });
  AppleMIDI.setHandleDisconnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc) {
    isConnected--;
    Serial.println("Disconnected");
  });
  
  MIDI.setHandleNoteOn([](byte channel, byte note, byte velocity) {
    Serial.println("NoteOn");
  });
  MIDI.setHandleNoteOff([](byte channel, byte note, byte velocity) {
    Serial.println("NoteOff");
  });

  Serial.println("Sending");

}

void sendNote(byte note, byte velocity, byte channel) {
    if (heldNotes.find(note) != heldNotes.end()) {
      MIDI.sendNoteOff(note, velocity, channel);
      heldNotes.erase(note);
    }

    MIDI.sendNoteOn(note, velocity, channel);
    heldNotes.insert(note);
}

void loop() {
  // Listen to incoming notes
  MIDI.read();

  // send a note every second
  // dont call delay() as it will stall the pipeline
  if ((isConnected > 0) && (millis() - t0) > 1000) {
    t0 = millis();

    sendNote(54, 55, 1);
  }

  //Serial.println(WiFi.localIP());
}
