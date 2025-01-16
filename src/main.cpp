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

/* Modules */
#include "modules/synth.h"
Synth synth;

/* Utils */
#include "utils/debug.h"
#include "utils/state.h"

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    // Debug::enable();
    Debug::println("Cranberry Synth");
    Debug::println("Mini Wavetable Synthesizer on Teensy 4.1");
}

void loop() {
    auto& led_state      = State::led_state;
    auto& led_state_prev = State::led_state_prev;
    auto& mode_state     = State::mode_state;

    while(true) {

        // 各モジュールの処理
        switch(mode_state) {
            case MODE_SYNTH:
                if(!midi_hdl.queueStatus()) midi_hdl.queueReset(true);
                synth.update();
                break;
        }

        // イベント監視・処理
        audio_hdl.process();
        midi_hdl.process();

        // LEDの切り替え
        if(led_state != led_state_prev) {
            led_state_prev = led_state;
            if(led_state) GPIO7_DR = CORE_PIN13_BITMASK;
            else GPIO7_DR = 0;
        }
    }
}