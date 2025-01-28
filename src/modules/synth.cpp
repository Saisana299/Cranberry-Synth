#include "modules/synth.h"

//todo modulationのenvはどう処理するか

/** @brief シンセ初期化 */
void Synth::init() {
    instance = this;
    for(uint8_t i = 0; i < MAX_NOTES; i++) {
        notes[i].order = 0;
        notes[i].note = 255;
        notes[i].velocity = 0;
        notes[i].channel = 0;

        for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
            notes[i].osc_mems[op] = Oscillator::Memory {0.0f, 0.0f, 0.0f};
            notes[i].env_mems[op] = Envelope::Memory {Envelope::State::Attack, 0, 0.0f, 0.0f};
        }
    }
    // [0]はCarrier確定
    operators[0].mode = OpMode::Carrier;
    operators[0].osc.enable();

    // [1]をモジュレーターに設定してみる
    operators[1].mode = OpMode::Modulator;
    operators[0].osc.setModulation(1, &operators[1].osc, &operators[1].env);
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
            auto& osc_mems = note.osc_mems;
            auto& env_mems = note.env_mems;
            auto& amp_level = State::amp_level;

            // オペレーター毎処理
            uint8_t carrier_cnt = 0;
            uint8_t r_finished_cnt = 0;
            for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
                auto& oper = operators[op];
                // Modulatorの更新処理はCarrier内で行う
                if(oper.mode == OpMode::Carrier && oper.osc.isActive()) {
                    ++carrier_cnt;
                    // todo:Carrierが2以上なら音量調節推奨
                    // サンプル毎処理
                    for(size_t i = 0; i < BUFFER_SIZE; ++i) {
                        // サンプル入手、ここでエンベロープレベルを適用
                        int16_t sample = oper.osc.getSample(osc_mems[op]);

                        // 合計を出力バッファに追加
                        samples_L[i] = samples_R[i] += sample * (amp_level * (1.0f / MAX_NOTES)) * oper.env.currentLevel(env_mems[op]);

                        // オシレーターとエンベロープを更新
                        oper.osc.update(osc_mems[op], &osc_mems[1], &env_mems[1]);
                        oper.env.update(env_mems[op]);
                    }

                    // Releaseが完了しているか
                    // モジュレータ―のエンベロープは考慮しない
                    if(oper.env.isFinished(env_mems[op])) {
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
    // 既に同じノートを演奏している場合
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if(notes[i].note == note) {
            for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
                // リリース中でなければ何もしない。
                if(notes[i].env_mems[op].state != Envelope::State::Release) return;
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
                oper.osc.setVolume(it.osc_mems[op], velocity);
                oper.osc.setFrequency(it.osc_mems[op], note);
                oper.osc.setPhase(it.osc_mems[op], 0.0f);
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
                oper.env.release(it.env_mems[op]);
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
        oper.osc.reset(it.osc_mems[op]);
        oper.env.reset(it.env_mems[op]);
    }
    if(order_max > 0) --order_max;
    // 他ノートorder更新
    updateOrder(removed);
}