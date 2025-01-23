#include "modules/synth.h"

//todo: 強制リリースはバッファサイズ分で振幅0まで滑らかに減衰するようにする 次のノートは減衰処理完了後に入れる（同ループ）

/** @brief シンセ初期化 */
void Synth::init() {
    instance = this;
    Oscillator::Memory osc_mem = {0.0f, 0.0f, 0.0f};
    Envelope::Memory env_mem = {Envelope::State::Attack, 0, 0.0f, 0.0f};
    for(uint8_t i = 0; i < MAX_NOTES; i++) {
        notes[i] = SynthNote {0, 255, 0, 0, osc_mem, env_mem};
    }
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
        if(notes[n].order != 0) {
            auto& note = notes[n];
            auto& osc_mem = note.osc_mem;
            auto& env_mem = note.env_mem;
            auto& amp_level = State::amp_level;

            // サンプル毎処理
            for(size_t i = 0; i < BUFFER_SIZE; ++i) {
                // サンプル入手
                int16_t sample = operators[0].osc.getSample(osc_mem);

                // 合計を出力バッファに追加
                samples_L[i] = samples_R[i] += sample * (amp_level * (1.0f / MAX_NOTES)) * operators[0].env.currentLevel(env_mem);

                // 位相を更新
                operators[0].osc.update(osc_mem);
                operators[0].env.update(env_mem);
            }

            // リリースが終わっていればリセット
            if(operators[0].env.isFinished(env_mem)) {
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
        if (notes[i].order > removed) {
            --notes[i].order;
            if(notes[i].order == 1) {
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
        if(notes[i].note == note) return;
    }
    // MAX_NOTES個ノートがある場合
    if(order_max == MAX_NOTES) {
        // 一番古いノートを強制停止する
        resetNote(last_index);
    }
    // ノートを追加する
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if (notes[i].order == 0) {
            auto& it = notes[i];
            if(order_max < MAX_NOTES) ++order_max;
            it.order = order_max;
            it.note = note;
            it.velocity = velocity;
            it.channel = channel;
            operators[0].osc.setVolume(it.osc_mem, velocity);
            operators[0].osc.setFrequency(it.osc_mem, note);
            operators[0].osc.setPhase(it.osc_mem);
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
        if (notes[i].note == note) {
            auto& it = notes[i];
            operators[0].env.release(it.env_mem);
        }
    }
}

/**
 * @brief ノートをリセット
 *
 * @param index リセットするノートのインデックス
 */
void Synth::resetNote(uint8_t index) {
    auto& it = notes[index];
    uint8_t removed = it.order;
    it.order = 0;
    it.note = 255;
    it.velocity = 0;
    it.channel = 0;
    operators[0].osc.reset(it.osc_mem);
    operators[0].env.reset(it.env_mem);
    if(order_max > 0) --order_max;
    // 他ノートorder更新
    updateOrder(removed);
}