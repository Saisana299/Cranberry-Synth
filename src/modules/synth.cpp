#include "modules/synth.h"

//todo: 強制リリースはバッファサイズ分で振幅0まで滑らかに減衰するようにする 次のノートは減衰処理完了後に入れる（同ループ）

/** @brief シンセ初期化 */
void Synth::init() {
    instance = this;
    for(uint8_t i = 0; i < MAX_NOTES; i++) {
        active_notes[i] = ActiveSynthNote {0, 255, 0, 0, Oscillator(), Envelope()};
    }
    float    sustain_level   = 1.0f;
    uint32_t attack_samples  = 0.001f * SAMPLE_RATE;
    uint32_t decay_samples   = 0.01f  * SAMPLE_RATE;
    uint32_t release_samples = 0.01f  * SAMPLE_RATE;
    amp_adsr = ADSRConfig {
        attack_samples,
        decay_samples,
        sustain_level,
        release_samples
    };
}

/** @brief シンセ生成 */
void Synth::generate() {
    if(samples_ready) return;

    // 出力バッファを初期化
    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
        samples_L[i] = 0;
        samples_R[i] = 0;
    }

    // ノート毎処理
    for(uint8_t n = 0; n < MAX_NOTES; ++n) {
        if(active_notes[n].order != 0) {
            auto& note = active_notes[n];
            auto& osc = note.osc;
            auto& amp_level = State::amp_level;
            auto& amp_env = note.amp_env;

            // サンプル毎処理
            for(size_t i = 0; i < BUFFER_SIZE; ++i) {
                // サンプル入手
                int16_t sample = osc.getSample();

                // 合計を出力バッファに追加
                samples_L[i] = samples_R[i] += sample * (amp_level * (1.0f / MAX_NOTES)) * amp_env.currentLevel();

                // 位相を更新
                osc.update();
                amp_env.update(amp_adsr, 1);
            }

            // リリースが終わっていればリセット
            if(amp_env.isFinished()) {
                resetNote(n);
            }
        }
    }

    samples_ready = true;
}

/** @brief シンセ更新 */
void Synth::update() {
    // 有効なノートが存在すれば生成
    // エフェクト系追加したら処理内容変更？
    if(order_max > 0) generate();
}

/**
 * @brief ノート整理番号の更新
 *
 * @param removed 削除したノートの整理番号
 */
void Synth::updateOrder(uint8_t removed) {
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if (active_notes[i].order > removed) {
            --active_notes[i].order;
            if(active_notes[i].order == 1) {
                last_index = i;
            }
        }
    }
}

/**
 * @brief 演奏するノートを追加します
 *
 * @param note MIDIノート番号
 * @param velocity MIDIベロシティ
 * @param channel MIDIチャンネル
 */
void Synth::noteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    // 既に演奏している場合
    // ONの次に同じ音のONはありえない状況
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if(active_notes[i].note == note) return;
    }
    // MAX_NOTES個ノートがある場合
    if(order_max == MAX_NOTES) {
        // 一番古いノートを強制停止する
        resetNote(last_index);
    }
    // ノートを追加する
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if (active_notes[i].order == 0) {
            auto& it = active_notes[i];
            if(order_max < MAX_NOTES) ++order_max;
            it.order = order_max;
            it.note = note;
            it.velocity = velocity;
            it.channel = channel;
            it.osc.setFrequency(note);
            it.osc.resetPhase();
            it.amp_env.reset();
            break;
        }
    }
}

/**
 * @brief ノートをリリースに移行
 *
 * @param note MIDIノート番号
 * @param channel MIDIチャンネル
 */
void Synth::noteOff(uint8_t note, uint8_t channel) {
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if (active_notes[i].note == note) {
            auto& it = active_notes[i];
            it.amp_env.release();
        }
    }
}

/**
 * @brief ノートをリセット
 *
 * @param index リセットするノートのインデックス
 */
void Synth::resetNote(uint8_t index) {
    auto& it = active_notes[index];
    uint8_t removed = it.order;
    it.order = 0;
    it.note = 255;
    it.velocity = 0;
    it.channel = 0;
    it.amp_env.reset();
    if(order_max > 0) --order_max;
    // 他ノートorder更新
    updateOrder(removed);
}