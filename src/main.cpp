/** Cranberry Synth **/
/** Mini Wavetable Synthesizer on Teensy 4.1 **/
/** @author Saisana299 **/

#include <Arduino.h>
#include <Adafruit_GFX.h>

/* Handlers */
#include "handlers/audio.hpp"
AudioHandler audio_hdl;
#include "handlers/midi.hpp"
MIDIHandler  midi_hdl;

/* Display */
#include "display/gfx.hpp"
GFX_SSD1351 gfx;

/* Modules */
#include "modules/synth.hpp"
Synth synth;

/* Utils */
#include "utils/debug.hpp"
#include "utils/state.hpp"
#include "utils/color.hpp"

/* dev1 */
#include "dev1/leds.hpp"
#include "dev1/switches.hpp"
Leds leds;
Switches switches;

void setup() {
    // Debug::enable();
    // Debug::println("Cranberry Synth");
    // Debug::println("Mini Wavetable Synthesizer on Teensy 4.1");

    gfx.begin();
    TextBounds bounds = gfx.getTextBounds("Cranberry Synth", 0, 12);
    const uint16_t canvas_h = static_cast<uint16_t>(bounds.y) + bounds.h;
    GFXcanvas16 canvas{bounds.w, canvas_h};
    gfx.drawString(canvas, "Cranberry Synth", 0, 0, Color::GRAY);
    gfx.drawString(canvas, "Dev-1", 0, 12, Color::GRAY);
    gfx.flash(canvas, 0, 0);
}

void loop() {
    auto& mode_state = State::mode_state;

    // /*debug*/ uint8_t count = 0;

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

        // /*debug*/ if(count == 16) {
        // /*debug*/     synth.debugFlash();
        // /*debug*/     count = 0;
        // /*debug*/ }
        // /*debug*/ ++count;
    }
}