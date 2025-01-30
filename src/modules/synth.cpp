#include "modules/synth.h"

/** @brief シンセ初期化 */
void Synth::init() {
    instance = this;
    // ノート情報を初期化
    for(uint8_t i = 0; i < MAX_NOTES; i++) {
        notes[i].order = 0;
        notes[i].note = 255;
        notes[i].velocity = 0;
        notes[i].channel = 0;
    }

    // オペレーター情報を初期化
    for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
        for(uint8_t i = 0; i < MAX_NOTES; ++i) {
            ope_states[op].osc_mems[i] = Oscillator::Memory {0, 0, 0.0f};
            ope_states[op].env_mems[i] = Envelope::Memory {Envelope::State::Attack, 0, 0.0f, 0.0f};
        }
    }

    // [0]はCarrier確定
    operators[0].mode = OpMode::Carrier;
    operators[0].osc.setLevel(1.0f);
    operators[0].osc.enable();

    // ディレイの実験
    delay.setDelay();
    delay_enabled = true;
}

/** @brief シンセ生成 */
void Synth::generate() {
    if(samples_ready) return;

    // サンプル毎処理
    for(size_t i = 0; i < BUFFER_SIZE; ++i) {
        // 出力バッファを初期化
        samples_L[i] = 0;
        samples_R[i] = 0;

        // ノート毎処理
        for(uint8_t n = 0; n < MAX_NOTES; ++n) {
            // 発音中のノート
            if(notes[n].order != 0) {
                uint8_t carrier_cnt = 0;
                uint8_t r_finished_cnt = 0;
                // オペレーター毎処理
                for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
                    auto& oper = operators[op];
                    auto& osc_mem = ope_states[op].osc_mems[n];
                    auto& env_mem = ope_states[op].env_mems[n];
                    // Modulatorの更新処理はCarrier内で行う
                    if(oper.mode == OpMode::Carrier && oper.osc.isActive()) {
                        ++carrier_cnt;

                        // サンプル入手、ここでエンベロープレベルを適用
                        // todo ステレオ対応
                        int16_t sample = oper.osc.getSample(osc_mem, n);

                        // 合計を出力バッファに追加
                        // ((サンプル*エンベロープ音量)*調整用レベル)*アンプレベル
                        // todo ステレオ対応
                        samples_L[i] = samples_R[i] += ((sample * oper.env.currentLevel(env_mem)) * adjust_level) * amp_level;

                        // オシレーターとエンベロープを更新
                        oper.osc.update(osc_mem, n);
                        oper.env.update(env_mem);

                        // Releaseが完了しているか
                        // モジュレータ―のエンベロープは考慮しない
                        if(oper.env.isFinished(env_mem)) {
                            ++r_finished_cnt;
                        }
                    }
                }

                // 全てのオペレーターが処理完了
                if(r_finished_cnt == carrier_cnt) {
                    resetNote(n);
                }
            }
        }

        // ディレイを適用
        if(delay_enabled) {
            samples_L[i] = delay.process(samples_L[i], false);
            samples_R[i] = delay.process(samples_R[i], true);
        }
    }

    samples_ready = true;
}

/** @brief シンセ更新 */
void Synth::update() {
    // 有効なノートが存在すれば生成
    // エフェクト系追加したら処理内容変更？
    if(order_max > 0) {
        delay_remain = delay.getDelayLength();
        generate();
    }
    // ディレイが残っている時の処理
    else if(delay_enabled) {
        if(delay_remain <= BUFFER_SIZE) delay_remain = 0;
        else delay_remain -= BUFFER_SIZE;
        generate();
    }
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
    // 既に同じノートを演奏している場合
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if(notes[i].note == note) {
            for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
                // リリース中でなければ何もしない。
                // if(ope_states[op].env_mems[i].state != Envelope::State::Release) return;
            }
            // リリース状態であれば強制リリース後発音 //todo
        }
    }
    // MAX_NOTES個ノートを演奏中の場合
    if(order_max == MAX_NOTES) {
        // 一番古いノートを強制停止する //todo: 強制リリース
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
                auto& osc_mem = ope_states[op].osc_mems[i];
                oper.osc.setVelocity(osc_mem, velocity);
                oper.osc.setFrequency(osc_mem, note);
                oper.osc.setPhase(osc_mem, 0);
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
            for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
                auto& oper = operators[op];
                auto& env_mem = ope_states[op].env_mems[i];
                oper.env.release(env_mem);
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
        auto& osc_mem = ope_states[op].osc_mems[index];
        auto& env_mem = ope_states[op].env_mems[index];
        oper.osc.reset(osc_mem);
        oper.env.reset(env_mem);
    }
    if(order_max > 0) --order_max;
    // 他ノートorder更新
    updateOrder(removed);
}