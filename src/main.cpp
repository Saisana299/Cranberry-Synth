/** Cranberry Synth **/
/** Mini FM Synthesizer on Teensy 4.1 **/
/** @author Saisana299 **/

// TODO: トラックが違うとリトリガーが正しく動かない問題を修正する（bitwigで確認済み）

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
#include "modules/passthrough.hpp"
#include "modules/delay.hpp"
#include "modules/filter.hpp"
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

// Synth / Passthrough 共有エフェクトインスタンス
// 両モードは同時に動作しないため、メモリ節約のため共有
Delay shared_delay;
Filter shared_filter;
Chorus shared_chorus;

Passthrough passthrough(audio_hdl, shared_filter, shared_delay, shared_chorus);

// SPI転送中のオーディオ処理コールバック
AudioCallback gfxAudioCallback = nullptr;

void audioProcessCallback() {
    if (state.getModeState() == MODE_PASSTHROUGH) {
        passthrough.process();
        return;
    }
    // 優先度の高い処理をSPI転送中も実行
    synth.update();        // サウンド生成
    audio_hdl.process();   // 音声信号処理
    midi_hdl.process();    // MIDI入力検知
    midi_player.process(); // MIDI Player 処理
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    // CrashReport: 前回クラッシュした場合、シリアル経由でレポートを出力
    if (CrashReport) {
        // シリアルが準備できるまで短時間待機
        Serial.begin(115200);
        delay(500);
        Serial.println("=== CRASH REPORT ===");
        Serial.print(CrashReport);
        Serial.println("====================");
        CrashReport.clear();
    }

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

    midi_player.init();  // SD.begin() はsetup()内で安全に呼ぶ
    synth.init(shared_delay, shared_filter, shared_chorus);
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
        // --- パススルーに入る ---
        if (mode_state == MODE_PASSTHROUGH) {
            midi_hdl.stop();       // MIDI受信を停止
            synth.reset();         // 発音中ノートをすべてリセット
            passthrough.begin();   // パススルー開始
        }
        // --- パススルーから抜ける ---
        if (last_mode == MODE_PASSTHROUGH && mode_state != MODE_PASSTHROUGH) {
            passthrough.end();     // パススルー停止 (バッファもフラッシュ)
        }
        // --- シンセモードに入る ---
        if (mode_state == MODE_SYNTH) {
            midi_hdl.begin();      // MIDI受信を再開
        }
        last_mode = mode_state;
    }

    // 優先度0: サウンド生成関連処理
    switch(mode_state) {
        case MODE_PASSTHROUGH: {
            // パススルーモード: 入力をそのまま出力
            passthrough.process();
            break;
        }
        case MODE_SYNTH: {
            // CPU使用率計測開始
            uint32_t t0 = ARM_DWT_CYCCNT;
            synth.update();
            uint32_t t1 = ARM_DWT_CYCCNT;

            // CPU使用率計算
            // Teensy 4.1: 600MHz, 1ブロック = 128サンプル @ 44100Hz ≈ 2.9ms
            // 2.9ms = 600MHz * 0.0029s = 1,740,000 cycles
            constexpr float CYCLES_PER_BLOCK = 600000000.0f / 44100.0f * 128.0f;
            // /*debug*/ constexpr float CYCLES_PER_US = 600.0f; // 600MHz = 600 cycles/μs
            float usage = (float)(t1 - t0) / CYCLES_PER_BLOCK * 100.0f;
            // /*debug*/ float elapsed_us = (float)(t1 - t0) / CYCLES_PER_US;

            // スムージング (急激な変化を抑える)
            static float smoothed_usage = 0.0f;
            // /*debug*/ static float smoothed_us = 0.0f;
            smoothed_usage = smoothed_usage * 0.9f + usage * 0.1f;
            // /*debug*/ smoothed_us = smoothed_us * 0.9f + elapsed_us * 0.1f;
            state.setCpuUsage(smoothed_usage);

            // // デバッグ出力 (500ms間隔)
            // /*debug*/ static uint32_t last_debug_time = 0;
            // /*debug*/ if (millis() - last_debug_time >= 500) {
            // /*debug*/     Serial.printf("[DEBUG] synth.update(): %.1f us (%.1f%% CPU)\n", smoothed_us, smoothed_usage);
            // /*debug*/     last_debug_time = millis();
            // /*debug*/ }
            break;
        }
    }

    // 優先度:1 音声信号処理(AD/DA)
    if (mode_state != MODE_PASSTHROUGH) {
        audio_hdl.process();
    }

    // 優先度:2 MIDI入力検知
    if (mode_state != MODE_PASSTHROUGH) {
        midi_hdl.process();
    }

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