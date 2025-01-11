/* Cranberry Synth */
/* Mini Wavetable Synthesizer on Teensy 4.1 */
#include <Arduino.h>

#include <handlers/audio.h>
#include <handlers/midi.h>
#include <display/gfx.h>

/* Handlers */
AudioHandler audio_hdl;
MIDIHandler  midi_hdl;

/* Display */
GFX_SSD1351 display;

void setup() {
    // Handler initialization
    audio_hdl.init();
    midi_hdl.init(audio_hdl);

    // Display initialization
    display.init();
}

void loop() {
    // process handlers
    audio_hdl.process();
    midi_hdl.process();
}