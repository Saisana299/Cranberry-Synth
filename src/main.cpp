/** Cranberry Synth **/
/** Mini Wavetable Synthesizer on Teensy 4.1 **/
/** @author Saisana299 **/

#include <Arduino.h>
#include <Entropy.h>
#include <Adafruit_GFX.h>
/* Handlers */
#include "handlers/audio.hpp"
#include "handlers/midi.hpp"
#include "handlers/file.hpp"
#include "handlers/serial.hpp"
#include "handlers/switches.hpp"
/* Display */
#include "display/gfx.hpp"
#include "display/leds.hpp"
/* UI */
#include "ui/screens/title.hpp"
#include "ui/ui.hpp"
/* Modules */
#include "modules/synth.hpp"
/* Utils */
#include "utils/state.hpp"
#include "utils/color.hpp"

/* インスタンス生成 */
State state;
AudioHandler audio_hdl(state);
MIDIHandler  midi_hdl(state);
FileHandler  file_hdl(state);
SerialHandler serial_hdl;
Switches switches(state);
GFX_SSD1351 gfx;
Leds leds(state);
UIManager ui(state);
Synth synth;

void setup() {
    serial_hdl.println("Cranberry Synth");
    serial_hdl.println("Digital FM Synthesizer on Teensy 4.1");

    gfx.begin();
    ui.pushScreen(new TitleScreen());

    Entropy.Initialize();
    randomSeed(Entropy.random());
}

void loop() {
    // 1ループ2900µs以内
    while(true) {
        auto mode_state = state.getModeState();

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