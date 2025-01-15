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
#include "utils/state.h"

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Debug::enable();
    Debug::println("Cranberry Synth");
    Debug::println("Mini Wavetable Synthesizer on Teensy 4.1");
    midi_hdl.queueMode(true);
}

void loop() {
    // Update handlers
    audio_hdl.process();
    midi_hdl.process();

    // Toggle LED
    if(State::led_state != State::led_state_prev) {
        State::led_state_prev = State::led_state;
        if(State::led_state) GPIO7_DR = CORE_PIN13_BITMASK;
        else GPIO7_DR = 0;
    }
}