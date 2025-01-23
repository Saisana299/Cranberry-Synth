#include "modules/synth.h"

/** @brief シンセ初期化 */
void Synth::init() {
    instance = this;
    Oscillator::Memory osc_mem = {0.0f, 0.0f, 0.0f};
    Envelope::Memory env_mem = {Envelope::State::Attack, 0, 0.0f, 0.0f};
    for(uint8_t i = 0; i < MAX_NOTES; i++) {
        notes[i] = SynthNote {0, 255, 0, 0, osc_mem, env_mem};
    }
    // [0]はCarrier確定
    operators[0].mode = OpMode::Carrier;
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

            // オペレーター毎処理
            uint8_t finished_cnt = 0;
            for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
                auto& oper = operators[op];
                // ModulatorはCarrier内で処理される
                if(oper.mode == OpMode::Carrier) {
                    // todo:Carrierが2以上なら音量調節推奨
                    // サンプル毎処理
                    for(size_t i = 0; i < BUFFER_SIZE; ++i) {
                        // サンプル入手
                        int16_t sample = oper.osc.getSample(osc_mem);

                        // 合計を出力バッファに追加
                        samples_L[i] = samples_R[i] += sample * (amp_level * (1.0f / MAX_NOTES)) * oper.env.currentLevel(env_mem);

                        // 位相を更新
                        oper.osc.update(osc_mem);
                        oper.env.update(env_mem);
                    }
                }

                // Releaseが完了しているか
                if(oper.env.isFinished(env_mem)) {
                    ++finished_cnt;
                }
            }

            // 全てのオペレーターが処理完了
            if(finished_cnt == MAX_OPERATORS) {
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
    // リリース状態であれば強制終了後発音//todo
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if(notes[i].note == note
        && notes[i].env_mem.state != Envelope::State::Release) return;
    }
    // MAX_NOTES個ノートがある場合
    if(order_max == MAX_NOTES) {
        // 一番古いノートを強制停止する//todo: 強制リリース
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
            for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
                auto& oper = operators[op];
                oper.osc.setVolume(it.osc_mem, velocity);
                oper.osc.setFrequency(it.osc_mem, note);
                oper.osc.setPhase(it.osc_mem);
            }
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
            for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
                auto& oper = operators[op];
                oper.env.release(it.env_mem);
            }
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
    for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
        auto& oper = operators[op];
        oper.osc.reset(it.osc_mem);
        oper.env.reset(it.env_mem);
    }
    if(order_max > 0) --order_max;
    // 他ノートorder更新
    updateOrder(removed);
}