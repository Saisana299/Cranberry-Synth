#include "modules/synth.hpp"

//todo 処理速度改善・処理方法変更
/**
4OP LPFあり 166μs
4OP LPFなし 140μs
LPF=26μs
1OP 67μs
2OP 93μs
3OP 121μs
0OP 37μs
発音中にリリースに入ると177μs
4OP 16notes LPFあり 2024μs
OPループ展開で16notes LPFあり 1922μs
*/

/** @brief シンセ初期化 */
void Synth::init() {
    // [0]はCarrier確定
    operators[0].mode = OpMode::Carrier;
    operators[0].osc.setLevel(1.0f);
    operators[0].osc.enable();

    operators[0].env.setDecay(0.4f);
    operators[0].env.setSustain(0.0f);
    operators[0].env.setRelease(0.3f);

    operators[0].osc.setDetune(3);

    // [1]を0のモジュレーターに //todo
    operators[1].mode = OpMode::Modulator;
    operators[0].osc.setModulation(&operators[1].osc, &operators[1].env, &ope_states[1].osc_mems[0], &ope_states[1].env_mems[0]);
    operators[1].osc.setLevelNonLinear(35);
    operators[1].osc.enable();

    operators[1].env.setDecay(0.2f);
    operators[1].env.setSustain(0.0f);

    operators[1].osc.setCoarse(14.0f);

    // [2]をCarrierに
    operators[2].mode = OpMode::Carrier;
    operators[2].osc.setLevel(1.0f);
    operators[2].osc.enable();

    operators[2].env.setDecay(2.0f);
    operators[2].env.setSustain(0.0f);
    operators[2].env.setRelease(0.3f);

    // [3]を2のモジュレーターに //todo
    operators[3].mode = OpMode::Modulator;
    operators[2].osc.setModulation(&operators[3].osc, &operators[3].env, &ope_states[3].osc_mems[0], &ope_states[3].env_mems[0]);
    operators[3].osc.setLevelNonLinear(89);
    operators[3].osc.enable();

    operators[3].env.setDecay(2.0f);
    operators[3].env.setSustain(0.0f);
    operators[3].env.setRelease(0.3f);

    // ローパスフィルタ
    filter.setLowPass(6000.0f, 1.0f/sqrt(2.0f));
    lpf_enabled = true;
}

/** @brief シンセ生成 */
void Synth::generate() {
    if(samples_ready) return;

    /*debug*/ uint32_t startTime = micros();

    // 定数キャッシュ
    const bool LPF_ENABLED = lpf_enabled;
    const bool HPF_ENABLED = hpf_enabled;
    const bool DELAY_ENABLED = delay_enabled;
    const float MASTER_SCALE = master_scale;

    // サンプル毎処理
    for(size_t i = 0; i < BUFFER_SIZE; ++i) {
        // 出力バッファ
        int32_t left = 0;
        int32_t right = 0;

        // ノート毎処理
        for(uint8_t n = 0; n < MAX_NOTES; ++n) {
            // 発音中でなければスキップ
            if(notes[n].order == 0) continue;

            uint8_t carrier_cnt = 0;
            uint8_t r_finished_cnt = 0;

            // // オペレーター毎処理
            // for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
            //     // ローカル参照
            //     Operator& oper = operators[op];
            //     auto& op_states_ref = ope_states[op];

            //     // Carrierかつアクティブでない場合はスキップ
            //     if(oper.mode != OpMode::Carrier || !oper.osc.isActive()) continue;

            //     ++carrier_cnt;
            //     auto& osc_mem = op_states_ref.osc_mems[n];
            //     auto& env_mem = op_states_ref.env_mems[n];

            //     // サンプル取得
            //     // todo ステレオ対応
            //     int16_t sample = oper.osc.getSample(osc_mem, n);

            //     // エンベロープレベル
            //     float env_level = oper.env.currentLevel(env_mem);

            //     // 演算順序の固定と乗算回数削減のため
            //     float scaled_sample = sample * env_level * MASTER_SCALE;

            //     // 合計を出力バッファに追加
            //     // ((サンプル*エンベロープ音量)*調整用レベル)*アンプレベル
            //     left += static_cast<int32_t>(scaled_sample);
            //     right += static_cast<int32_t>(scaled_sample);

            //     // オシレーターとエンベロープを更新
            //     oper.osc.update(osc_mem, n);
            //     oper.env.update(env_mem);

            //     // Releaseが完了しているか
            //     // モジュレータ―のエンベロープは考慮しない
            //     if(oper.env.isFinished(env_mem)) {
            //         ++r_finished_cnt;
            //     }
            // } // for operator

            // オペレーター0
            {
                Operator& oper0 = operators[0];
                auto& state0 = ope_states[0];
                if (oper0.mode == OpMode::Carrier && oper0.osc.isActive()) {
                    ++carrier_cnt;
                    auto& osc_mem0 = state0.osc_mems[n];
                    auto& env_mem0 = state0.env_mems[n];

                    int16_t sample = oper0.osc.getSample(osc_mem0, n);
                    float env_level = oper0.env.currentLevel(env_mem0);
                    float scaled_sample = sample * env_level * MASTER_SCALE;

                    left  += static_cast<int32_t>(scaled_sample);
                    right += static_cast<int32_t>(scaled_sample);

                    oper0.osc.update(osc_mem0, n);
                    oper0.env.update(env_mem0);

                    if (oper0.env.isFinished(env_mem0)) {
                        ++r_finished_cnt;
                    }
                }
            }

            // オペレーター1
            {
                Operator& oper1 = operators[1];
                auto& state1 = ope_states[1];
                if (oper1.mode == OpMode::Carrier && oper1.osc.isActive()) {
                    ++carrier_cnt;
                    auto& osc_mem1 = state1.osc_mems[n];
                    auto& env_mem1 = state1.env_mems[n];

                    int16_t sample = oper1.osc.getSample(osc_mem1, n);
                    float env_level = oper1.env.currentLevel(env_mem1);
                    float scaled_sample = sample * env_level * MASTER_SCALE;

                    left  += static_cast<int32_t>(scaled_sample);
                    right += static_cast<int32_t>(scaled_sample);

                    oper1.osc.update(osc_mem1, n);
                    oper1.env.update(env_mem1);

                    if (oper1.env.isFinished(env_mem1)) {
                        ++r_finished_cnt;
                    }
                }
            }

            // オペレーター2
            {
                Operator& oper2 = operators[2];
                auto& state2 = ope_states[2];
                if (oper2.mode == OpMode::Carrier && oper2.osc.isActive()) {
                    ++carrier_cnt;
                    auto& osc_mem2 = state2.osc_mems[n];
                    auto& env_mem2 = state2.env_mems[n];

                    int16_t sample = oper2.osc.getSample(osc_mem2, n);
                    float env_level = oper2.env.currentLevel(env_mem2);
                    float scaled_sample = sample * env_level * MASTER_SCALE;

                    left  += static_cast<int32_t>(scaled_sample);
                    right += static_cast<int32_t>(scaled_sample);

                    oper2.osc.update(osc_mem2, n);
                    oper2.env.update(env_mem2);

                    if (oper2.env.isFinished(env_mem2)) {
                        ++r_finished_cnt;
                    }
                }
            }

            // オペレーター3
            {
                Operator& oper3 = operators[3];
                auto& state3 = ope_states[3];
                if (oper3.mode == OpMode::Carrier && oper3.osc.isActive()) {
                    ++carrier_cnt;
                    auto& osc_mem3 = state3.osc_mems[n];
                    auto& env_mem3 = state3.env_mems[n];

                    int16_t sample = oper3.osc.getSample(osc_mem3, n);
                    float env_level = oper3.env.currentLevel(env_mem3);
                    float scaled_sample = sample * env_level * MASTER_SCALE;

                    left  += static_cast<int32_t>(scaled_sample);
                    right += static_cast<int32_t>(scaled_sample);

                    oper3.osc.update(osc_mem3, n);
                    oper3.env.update(env_mem3);

                    if (oper3.env.isFinished(env_mem3)) {
                        ++r_finished_cnt;
                    }
                }
            }

            // 全てのオペレーターが処理完了
            if(r_finished_cnt == carrier_cnt) {
                resetNote(n);
            }
        } // for active note

        // サンプルをクリッピングする
        left  = AudioMath::fastClampInt16(left);
        right = AudioMath::fastClampInt16(right);
        // フィルタはそれぞれでクリッピングがあるためこれ以降は必要無し

        // LPFを適用
        if(LPF_ENABLED) {
            left = filter.processLpf(left, false);
            right = filter.processLpf(right, true);
        }

        // HPFを適用
        if(HPF_ENABLED) {
            left = filter.processHpf(left, false);
            right = filter.processHpf(right, true);
        }

        // ディレイを適用
        if(DELAY_ENABLED) {
            left = delay.process(left, false);
            right = delay.process(right, true);
        }

        samples_L[i] = static_cast<int16_t>(left);
        samples_R[i] = static_cast<int16_t>(right);

    } // for BUFFER_SIZE

    samples_ready = true;

    /*debug*/ uint32_t endTime = micros();
    /*debug*/ uint32_t duration = endTime - startTime;
    /*debug*/ Serial.println(String(duration) + "us");
    // 2900μs以内に終わらせる必要がある
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
    else if(delay_enabled && delay_remain > 0) {
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
                auto& osc_mem = ope_states[op].osc_mems[i];
                operators[op].osc.setVelocity(osc_mem, velocity);
                operators[op].osc.setFrequency(osc_mem, note);
                operators[op].osc.setPhase(osc_mem, 0);
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
    uint8_t removed_order = it.order;
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
    updateOrder(removed_order);
}