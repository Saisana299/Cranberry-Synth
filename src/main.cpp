/** Cranberry Synth **/
/** Mini Wavetable Synthesizer on Teensy 4.1 **/
/** @author Saisana299 **/

#include <Arduino.h>

/* Handlers */
#include "handlers/audio.hpp"
AudioHandler audio_hdl;
#include "handlers/midi.hpp"
MIDIHandler  midi_hdl;

/* Display */
#include "display/gfx.hpp"
GFX_SSD1351  display;

/* Modules */
#include "modules/synth.hpp"
Synth synth;

/* Utils */
#include "utils/debug.hpp"
#include "utils/state.hpp"

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    // Debug::enable();
    // Debug::println("Cranberry Synth");
    // Debug::println("Mini Wavetable Synthesizer on Teensy 4.1");
}

void loop() {
    auto& led_state      = State::led_state;
    auto& led_state_prev = State::led_state_prev;
    auto& mode_state     = State::mode_state;

    while(true) {

        // イベント監視・処理
        midi_hdl.process();
        audio_hdl.process();

        // 各モジュールの処理
        switch(mode_state) {
        case MODE_SYNTH:
            synth.update();
            break;
        }

        // LEDの切り替え
        if(led_state != led_state_prev) {
            led_state_prev = led_state;
            if(led_state) GPIO7_DR = CORE_PIN13_BITMASK;
            else GPIO7_DR = 0;
        }
    }
}