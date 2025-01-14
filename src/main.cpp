/** Cranberry Synth **/
/** Mini Wavetable Synthesizer on Teensy 4.1 **/
#include <Arduino.h>

/* Handlers */
#include "handlers/audio.h"
AudioHandler audio_hdl;
#include "handlers/midi.h"
MIDIHandler  midi_hdl;

/* Display */
#include "display/gfx.h"
GFX_SSD1351  display;

/* Utils */
#include "utils/debug.h"

void setup() {
    Debug::println("Cranberry Synth");
}

void loop() {
    audio_hdl.process();
    midi_hdl.process();
}