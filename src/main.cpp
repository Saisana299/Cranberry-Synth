/** Cranberry Synth **/
/** Mini FM Synthesizer on Teensy 4.1 **/
/** @author Saisana299 **/

//TODO: RATE SCALING設定は将来的に実装予定

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
/* Modules */
#include "modules/synth.hpp"
/* UI */
#include "ui/ui.hpp"
#include "ui/screens/title.hpp"
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

// SPI転送中のオーディオ処理コールバック
AudioCallback gfxAudioCallback = nullptr;

void audioProcessCallback() {
    // 優先度の高い処理をSPI転送中も実行
    synth.update();        // サウンド生成
    audio_hdl.process();   // 音声信号処理
    midi_hdl.process();    // MIDI入力検知
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    for(int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
    }

    digitalWrite(LED_BUILTIN, HIGH);

    randomSeed(Entropy.random());

    serial_hdl.begin();
    gfx.begin();
    ui.pushScreen(new TitleScreen());

    synth.init();
    audio_hdl.init();
    physical.init();
    leds.init();

    // SPI転送中のオーディオコールバックを設定
    gfxAudioCallback = audioProcessCallback;

    // オーディオ割り込み優先度を最高に
    NVIC_SET_PRIORITY(IRQ_SAI1, 0); // Teensy 4.1

    digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
    static uint8_t last_mode = state.getModeState();
    auto mode_state = state.getModeState();

    if (mode_state != last_mode) {
        // SYNTHになったらMIDI入力を開始
        if (mode_state == MODE_SYNTH) {
            midi_hdl.begin();
        }
        last_mode = mode_state;
    }

    // 優先度0: サウンド生成関連処理
    switch(mode_state) {
        case MODE_SYNTH:
            // /*debug*/ uint32_t t0 = ARM_DWT_CYCCNT;
            synth.update();
            // /*debug*/ uint32_t t1 = ARM_DWT_CYCCNT;
            // /*debug*/ Serial.println((t1 - t0) / 600);
            // sine波1音+LPFで62µs以内目標
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

    asm volatile("yield");
}