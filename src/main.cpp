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
#include "handlers/file.hpp"
FileHandler  file_hdl;
#include "handlers/serial.hpp"
SerialHandler serial_hdl;
#include "handlers/switches.hpp"
Switches switches;

/* Display */
#include "display/gfx.hpp"
GFX_SSD1351 gfx;
#include "display/leds.hpp"
Leds leds;

/* UI */
#include "ui/screens/title.hpp"
#include "ui/ui.hpp"
UIManager ui;

/* Modules */
#include "modules/synth.hpp"
Synth synth;

/* Utils */
#include "utils/state.hpp"
#include "utils/color.hpp"

void setup() {
    serial_hdl.println("Cranberry Synth");
    serial_hdl.println("Digital FM Synthesizer on Teensy 4.1");

    gfx.begin();
    ui.pushScreen(new TitleScreen());

    randomSeed(analogRead(0));
}

void loop() {
    auto& mode_state = State::mode_state;

    while(true) {

        // 優先度0: サウンド生成関連処理
        switch(mode_state) {
        case MODE_SYNTH:
            synth.update();
            break;
        }

        // 優先度:1 音声信号処理(AD/DA)
        audio_hdl.process();

        // 優先度:2 MIDI入力検知
        midi_hdl.process();

        // 優先度:3 スイッチ処理
        switches.process();

        // 優先度:4 UI処理
        ui.render();

        // 優先度:5 MIDI Player 処理
        file_hdl.process();

        // 優先度:6 シリアル通信処理(USB)
        serial_hdl.process();

        // 優先度:7 LED制御
        leds.process();
    }
}