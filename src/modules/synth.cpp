#include "modules/synth.hpp"

/** @brief シンセ初期化 */
void Synth::init() {

    // ノート情報クリア
    for (int i = 0; i < 128; ++i) {
        midi_note_to_index[i] = -1;
    }

    // [0]はCarrier確定
    operators[0].mode = OpMode::Carrier;
    operators[0].osc.setLevel(1024);
    operators[0].osc.enable();

    operators[0].env.setDecay(60);
    operators[0].env.setSustain(0);
    operators[0].env.setRelease(80);

    operators[0].osc.setDetune(3);

    // [1]を0のモジュレーターに //TODO
    operators[1].mode = OpMode::Modulator;
    operators[0].osc.setModulation(&operators[1].osc, &operators[1].env, &ope_states[1].osc_mems[0], &ope_states[1].env_mems[0]);
    operators[1].osc.setLevelNonLinear(35);
    operators[1].osc.enable();

    operators[1].env.setDecay(60);
    operators[1].env.setSustain(0);

    operators[1].osc.setCoarse(14.0f);

    // [2]をCarrierに
    operators[2].mode = OpMode::Carrier;
    operators[2].osc.setLevel(1024);
    operators[2].osc.enable();

    operators[2].env.setDecay(94);
    operators[2].env.setSustain(0);
    operators[2].env.setRelease(80);

    // [3]を2のモジュレーターに //TODO
    operators[3].mode = OpMode::Modulator;
    operators[2].osc.setModulation(&operators[3].osc, &operators[3].env, &ope_states[3].osc_mems[0], &ope_states[3].env_mems[0]);
    operators[3].osc.setLevelNonLinear(89);
    operators[3].osc.enable();

    operators[3].env.setDecay(94);
    operators[3].env.setSustain(0);
    operators[3].env.setRelease(80);

    // [4]をCarrierに
    operators[4].mode = OpMode::Carrier;
    operators[4].osc.setLevel(1024);
    operators[4].osc.enable();

    operators[4].env.setDecay(94);
    operators[4].env.setSustain(0);
    operators[4].env.setRelease(80);

    operators[4].osc.setDetune(-7);

    // [5]を4のモジュレーターに //TODO
    operators[5].mode = OpMode::Modulator;
    operators[4].osc.setModulation(&operators[5].osc, &operators[5].env, &ope_states[5].osc_mems[0], &ope_states[5].env_mems[0]);
    operators[5].osc.setLevelNonLinear(79);
    operators[5].osc.enable();

    operators[5].env.setDecay(94);
    operators[5].env.setSustain(0);
    operators[5].env.setRelease(80);

    operators[5].osc.setDetune(+7);

    // ローパスフィルタ
    filter.setLowPass(6000.0f, 1.0f/sqrt(2.0f));
    lpf_enabled = true;

    // キャリア数が3
    master_scale = (static_cast<uint32_t>(amp_level / 3) * adjust_level) >> 10;
}

/** @brief シンセ生成 */
/** @attention ここで呼び出す関数は全てinlineで実装する */
/** envelopeのupdateはinline不可 */
void Synth::generate() {
    if(samples_ready_flags != 0) return;

    // /*debug*/ uint32_t startTime = micros();

    // 定数キャッシュ
    const bool LPF_ENABLED = lpf_enabled;
    const bool HPF_ENABLED = hpf_enabled;
    const bool DELAY_ENABLED = delay_enabled;
    const int16_t MASTER_SCALE = master_scale;
    const int16_t MASTER_PAN = master_pan;

    // サンプル毎処理
    for(size_t i = 0; i < BUFFER_SIZE; ++i) {
        // 出力バッファ
        int32_t left = 0;
        int32_t right = 0;

        //TODO discordに記載

        // ノート毎処理
        for(uint8_t n = 0; n < MAX_NOTES; ++n) {
            // 発音中でなければスキップ
            if(notes[n].order == 0) continue;

            uint8_t carrier_cnt = 0;
            uint8_t r_finished_cnt = 0;

            // オペレーター処理 手動ループ展開
            // キャリアの場合のみここで処理を行う

            //TODO ノートのチャンネルを特定して処理する

            // オペレーター1
            {
                Operator& oper1 = operators[0];
                auto& state1 = ope_states[0];
                if (oper1.mode == OpMode::Carrier && oper1.osc.isActive()) {
                    ++carrier_cnt;
                    auto& osc_mem1 = state1.osc_mems[n];
                    auto& env_mem1 = state1.env_mems[n];

                    const int32_t sample = static_cast<int32_t>(oper1.osc.getSample(osc_mem1, n));
                    const int32_t env_level = static_cast<int32_t>(oper1.env.currentLevel(env_mem1));
                    const int32_t enved_sample = (sample * env_level) >> 10;
                    const int32_t scaled_sample = (enved_sample * MASTER_SCALE) >> 10;

                    left  += scaled_sample;
                    right += scaled_sample;

                    oper1.osc.update(osc_mem1, n);
                    oper1.env.update(env_mem1);

                    if (oper1.env.isFinished(env_mem1)) {
                        ++r_finished_cnt;
                    }
                }
            }

            // オペレーター2
            {
                Operator& oper2 = operators[1];
                auto& state2 = ope_states[1];
                if (oper2.mode == OpMode::Carrier && oper2.osc.isActive()) {
                    ++carrier_cnt;
                    auto& osc_mem2 = state2.osc_mems[n];
                    auto& env_mem2 = state2.env_mems[n];

                    const int32_t sample = static_cast<int32_t>(oper2.osc.getSample(osc_mem2, n));
                    const int32_t env_level = static_cast<int32_t>(oper2.env.currentLevel(env_mem2));
                    const int32_t enved_sample = (sample * env_level) >> 10;
                    const int32_t scaled_sample = (enved_sample * MASTER_SCALE) >> 10;

                    left  += scaled_sample;
                    right += scaled_sample;

                    oper2.osc.update(osc_mem2, n);
                    oper2.env.update(env_mem2);

                    if (oper2.env.isFinished(env_mem2)) {
                        ++r_finished_cnt;
                    }
                }
            }

            // オペレーター3
            {
                Operator& oper3 = operators[2];
                auto& state3 = ope_states[2];
                if (oper3.mode == OpMode::Carrier && oper3.osc.isActive()) {
                    ++carrier_cnt;
                    auto& osc_mem3 = state3.osc_mems[n];
                    auto& env_mem3 = state3.env_mems[n];

                    const int32_t sample = static_cast<int32_t>(oper3.osc.getSample(osc_mem3, n));
                    const int32_t env_level = static_cast<int32_t>(oper3.env.currentLevel(env_mem3));
                    const int32_t enved_sample = (sample * env_level) >> 10;
                    const int32_t scaled_sample = (enved_sample * MASTER_SCALE) >> 10;

                    left  += scaled_sample;
                    right += scaled_sample;

                    oper3.osc.update(osc_mem3, n);
                    oper3.env.update(env_mem3);

                    if (oper3.env.isFinished(env_mem3)) {
                        ++r_finished_cnt;
                    }
                }
            }

            // オペレーター4
            {
                Operator& oper4 = operators[3];
                auto& state4 = ope_states[3];
                if (oper4.mode == OpMode::Carrier && oper4.osc.isActive()) {
                    ++carrier_cnt;
                    auto& osc_mem4 = state4.osc_mems[n];
                    auto& env_mem4 = state4.env_mems[n];

                    const int32_t sample = static_cast<int32_t>(oper4.osc.getSample(osc_mem4, n));
                    const int32_t env_level = static_cast<int32_t>(oper4.env.currentLevel(env_mem4));
                    const int32_t enved_sample = (sample * env_level) >> 10;
                    const int32_t scaled_sample = (enved_sample * MASTER_SCALE) >> 10;

                    left  += scaled_sample;
                    right += scaled_sample;

                    oper4.osc.update(osc_mem4, n);
                    oper4.env.update(env_mem4);

                    if (oper4.env.isFinished(env_mem4)) {
                        ++r_finished_cnt;
                    }
                }
            }

            // オペレーター5
            {
                Operator& oper5 = operators[4];
                auto& state5 = ope_states[4];
                if (oper5.mode == OpMode::Carrier && oper5.osc.isActive()) {
                    ++carrier_cnt;
                    auto& osc_mem5 = state5.osc_mems[n];
                    auto& env_mem5 = state5.env_mems[n];

                    const int32_t sample = static_cast<int32_t>(oper5.osc.getSample(osc_mem5, n));
                    const int32_t env_level = static_cast<int32_t>(oper5.env.currentLevel(env_mem5));
                    const int32_t enved_sample = (sample * env_level) >> 10;
                    const int32_t scaled_sample = (enved_sample * MASTER_SCALE) >> 10;

                    left  += scaled_sample;
                    right += scaled_sample;

                    oper5.osc.update(osc_mem5, n);
                    oper5.env.update(env_mem5);

                    if (oper5.env.isFinished(env_mem5)) {
                        ++r_finished_cnt;
                    }
                }
            }

            // オペレーター6
            {
                Operator& oper6 = operators[5];
                auto& state6 = ope_states[5];
                if (oper6.mode == OpMode::Carrier && oper6.osc.isActive()) {
                    ++carrier_cnt;
                    auto& osc_mem6 = state6.osc_mems[n];
                    auto& env_mem6 = state6.env_mems[n];

                    const int32_t sample = static_cast<int32_t>(oper6.osc.getSample(osc_mem6, n));
                    const int32_t env_level = static_cast<int32_t>(oper6.env.currentLevel(env_mem6));
                    const int32_t enved_sample = (sample * env_level) >> 10;
                    const int32_t scaled_sample = (enved_sample * MASTER_SCALE) >> 10;

                    left  += scaled_sample;
                    right += scaled_sample;

                    oper6.osc.update(osc_mem6, n);
                    oper6.env.update(env_mem6);

                    if (oper6.env.isFinished(env_mem6)) {
                        ++r_finished_cnt;
                    }
                }
            }

            // 全てのオペレーターが処理完了
            if(r_finished_cnt == carrier_cnt) {
                noteReset(n);
            }
        } // for active note

        // サンプルをクリッピングする
        left  = AudioMath::fastClampInt16(left);
        right = AudioMath::fastClampInt16(right);
        // フィルタはそれぞれでクリッピングがあるためこれ以降は必要無し

        // LPFを適用
        if(LPF_ENABLED) {
            left = filter.processLpfL(left);
            right = filter.processLpfR(right);
        }

        // HPFを適用
        if(HPF_ENABLED) {
            left = filter.processHpfL(left);
            right = filter.processHpfR(right);
        }

        // ディレイを適用
        if(DELAY_ENABLED) {
            left = delay.processL(left);
            right = delay.processR(right);
        }

        // パンを適用
        left  = (left  * AudioMath::PAN_COS_TABLE[MASTER_PAN]) / INT16_MAX;
        right = (right * AudioMath::PAN_SIN_TABLE[MASTER_PAN]) / INT16_MAX;

        samples_L[i] = static_cast<int16_t>(left);
        samples_R[i] = static_cast<int16_t>(right);

        // バランス接続用反転
        // left/rightは既にクリッピングされているため-32768～32767の範囲に収まっている
        if(left == INT16_MIN) {
            samples_LM[i] = INT16_MAX;
        } else {
            samples_LM[i] = static_cast<int16_t>(-left);
        }

        if(right == INT16_MIN) {
            samples_RM[i] = INT16_MAX;
        } else {
            samples_RM[i] = static_cast<int16_t>(-right);
        }

    } // for BUFFER_SIZE

    samples_ready_flags = 1;

    // /*debug*/ uint32_t endTime = micros();
    // /*debug*/ uint32_t duration = endTime - startTime;
    // /*debug*/ Serial.println(String(duration) + "us");
    // 2900μs以内に終わらせる必要がある
    // sine波1音+LPFで62µs以内目標
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
    if(midi_note_to_index[note] != -1) {
        //for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
            // リリース中でなければ何もしない。
            // if(ope_states[op].env_mems[i].state != Envelope::EnvelopeState::Release) return;
        //}
        // リリース状態であれば強制リリース後発音 //TODO: 強制リリース
    }
    // MAX_NOTES個ノートを演奏中の場合
    if(order_max == MAX_NOTES) {
        // 一番古いノートを強制停止する //TODO: 強制リリース
        noteReset(last_index);
    }
    // ノートを追加する
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if (notes[i].order == 0) {
            midi_note_to_index[note] = i;
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
    if (midi_note_to_index[note] != -1) {
        uint8_t i = midi_note_to_index[note];
        for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
            auto& oper = operators[op];
            auto& env_mem = ope_states[op].env_mems[i];
            oper.env.release(env_mem);
        }
    }
}

/**
 * @brief ノートをリセット
 *
 * @param index リセットするノートのインデックス
 */
void Synth::noteReset(uint8_t index) {
    auto& it = notes[index];
    uint8_t removed_order = it.order;
    midi_note_to_index[it.note] = -1;
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

void Synth::reset() {
    for(uint8_t i = 0; i < MAX_NOTES; ++i) {
        noteReset(i);
    }
}