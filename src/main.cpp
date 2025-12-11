/** Cranberry Synth **/
/** Mini Wavetable Synthesizer on Teensy 4.1 **/
/** @author Saisana299 **/

#include <Arduino.h>
#include <Entropy.h>
#include <Adafruit_GFX.h>
/* Handlers */
#include "handlers/audio.hpp"
#include "handlers/midi.hpp"
#include "handlers/serial.hpp"
#include "handlers/physical.hpp"
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
/* Tools */
#include "tools/midi_player.hpp"

/* インスタンス生成 */
State state;
AudioHandler audio_hdl(state);
MIDIHandler  midi_hdl(state);
MIDIPlayer  midi_player(state);
PhysicalHandler physical(state);
GFX_SSD1351 gfx;
Leds leds(state);
UIManager ui(state);

Synth& synth = Synth::getInstance();

void setup() {
    serial_hdl.begin();
    serial_hdl.println("Cranberry Synth");
    serial_hdl.println("Digital FM Synthesizer on Teensy 4.1");

    gfx.begin();
    ui.pushScreen(new TitleScreen());

    randomSeed(Entropy.random());

    synth.init();
    audio_hdl.init();
    midi_hdl.init();
    physical.init();
    leds.init();
}

void loop() {
    // 1ループ2900μs以内
    // /*debug*/ uint32_t startTime = micros();

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

    // 優先度:3 物理ボタン処理
    physical.process();

    // 優先度:4 UI処理
    ui.render();

    // 優先度:5 MIDI Player 処理
    midi_player.process();

    // 優先度:6 シリアル通信処理(USB)
    serial_hdl.process();

    // 優先度:7 LED制御
    leds.process();

    // /*debug*/ uint32_t endTime = micros();
    // /*debug*/ uint32_t duration = endTime - startTime;
    // /*debug*/ Serial.println(String(duration) + "us");
    // sine波1音+LPFで62µs以内目標

    asm volatile("yield");
}