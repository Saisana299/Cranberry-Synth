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

/* dev1 */
#include "dev1/leds.hpp"
#include "dev1/switches.hpp"
Leds leds;
Switches switches;

void setup() {
    // Debug::enable();
    // Debug::println("Cranberry Synth");
    // Debug::println("Mini Wavetable Synthesizer on Teensy 4.1");
}

void loop() {
    auto& mode_state = State::mode_state;

    while(true) {
        // イベント監視・処理
        midi_hdl.process();
        audio_hdl.process();

        // dev1
        leds.process();
        switches.process();

        // 各モジュールの処理
        switch(mode_state) {
        case MODE_SYNTH:
            synth.update();
            break;
        }
    }
}