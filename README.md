Uses an esp32 to handle drum triggers from an acoustic drumset and send midi outputs.

First copy the secrets template and edit the values:
- `cd midi_trigger`
- `cp secrets.h.template secrets.h`

Edit the values in secrets.h to match your WiFi credentials.

Then open `midi_trigger.ino` in Arduino IDE, install the required libraries, then run it.

Required libraries:
- AppleMidi
- Bounce2
