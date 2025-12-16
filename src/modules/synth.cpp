#include "modules/synth.hpp"

/** @brief シンセ初期化 */
void Synth::init() {

    // ノート情報クリア
    for (int i = 0; i < 128; ++i) {
        midi_note_to_index[i] = -1;
    }
    Oscillator::initTable();

    setAlgorithm(0);
    setFeedback(0);

    // --- ペア1: Op1(Mod) -> Op0(Car) ---
    // Carrier: Op0
    operators[0].osc.setLevel(1024);
    operators[0].osc.enable();
    operators[0].osc.setDetune(3);
    operators[0].env.setDecay(60);
    operators[0].env.setSustain(0);
    operators[0].env.setRelease(80);

    // Modulator: Op1
    operators[1].osc.setLevelNonLinear(35);
    operators[1].osc.enable();
    operators[1].osc.setCoarse(14.0f);
    operators[1].env.setDecay(60);
    operators[1].env.setSustain(0);

    // --- ペア2: Op3(Mod) -> Op2(Car) ---
    // Carrier: Op2
    operators[2].osc.setLevel(1024);
    operators[2].osc.enable();
    operators[2].env.setDecay(94);
    operators[2].env.setSustain(0);
    operators[2].env.setRelease(80);

    // Modulator: Op3
    operators[3].osc.setLevelNonLinear(89);
    operators[3].osc.enable();
    operators[3].env.setDecay(94);
    operators[3].env.setSustain(0);
    operators[3].env.setRelease(80);

    // --- ペア3: Op5(Mod) -> Op4(Car) ---
    // Carrier: Op4
    operators[4].osc.setLevel(1024);
    operators[4].osc.enable();
    operators[4].osc.setDetune(-7);
    operators[4].env.setDecay(94);
    operators[4].env.setSustain(0);
    operators[4].env.setRelease(80);

    // Modulator: Op5
    operators[5].osc.setLevelNonLinear(79);
    operators[5].osc.enable();
    operators[5].osc.setDetune(+7);
    operators[5].env.setDecay(94);
    operators[5].env.setSustain(0);
    operators[5].env.setRelease(80);

    // ローパスフィルタ
    filter.setLowPass(6000.0f, 1.0f/sqrt(2.0f));
    lpf_enabled = true;

    // キャリア数が3
    master_scale = (static_cast<uint32_t>(amp_level / 3) * adjust_level) >> 10;
}

/** @brief シンセ生成 */
FASTRUN void Synth::generate() {
    if(samples_ready_flags != 0) return;
    if(current_algo == nullptr) return;

    // 定数キャッシュ
    const uint8_t* exec_order = current_algo->exec_order;
    const uint8_t* mod_mask = current_algo->mod_mask;
    const uint8_t output_mask = current_algo->output_mask;
    const int8_t feedback_op = current_algo->feedback_op;

    static const uint8_t FB_SHIFTS[8] = { 31, 7, 6, 5, 4, 3, 2, 1 };
    uint8_t current_fb_shift = (feedback_amount > 7) ? 31 : FB_SHIFTS[feedback_amount];

    // 出力バッファをクリア
    int32_t mix_buffer_L[BUFFER_SIZE] = {0};
    int32_t mix_buffer_R[BUFFER_SIZE] = {0};

    // ノート毎処理
    for(uint8_t n = 0; n < MAX_NOTES; ++n) {
        if(notes[n].order == 0) continue;

        bool note_is_active = false;

        // オペレータ出力の一時保存用バッファ [Op][Sample]
        int32_t op_buffer[MAX_OPERATORS][BUFFER_SIZE];

        // --- オペレータ毎処理 ---
        // #pragma GCC unroll 6
        for(uint8_t k = 0; k < MAX_OPERATORS; ++k) {
            uint8_t op_idx = exec_order[k];
            Operator& op_obj = operators[op_idx];

            // ステートへの参照キャッシュ
            auto& osc_mem = ope_states[op_idx].osc_mems[n];
            auto& env_mem = ope_states[op_idx].env_mems[n];

            uint8_t mask = mod_mask[op_idx];
            bool is_feedback = (op_idx == feedback_op && feedback_amount > 0);

            // フィードバック変数のローカルキャッシュ
            int32_t fb_h0 = fb_history[n][0];
            int32_t fb_h1 = fb_history[n][1];

            // --- サンプル毎処理 ---
            for(size_t i = 0; i < BUFFER_SIZE; ++i) {
                int32_t mod_input = 0;

                // 1. 変調入力
                if (mask) {
                    for(uint8_t src = 0; src < MAX_OPERATORS; ++src) {
                         if (mask & (1 << src)) {
                             mod_input += op_buffer[src][i]; // 計算済みのバッファから読む
                         }
                    }
                }

                // 2. フィードバック
                if (is_feedback) {
                    int32_t fb_val = (fb_h0 + fb_h1) >> 1;
                    if (current_fb_shift < 30) {
                        mod_input += (fb_val >> current_fb_shift);
                    }
                }

                // 3. 発音
                int16_t raw_wave = op_obj.osc.getSample(osc_mem, mod_input);
                int32_t env_level = op_obj.env.currentLevel(env_mem);
                int32_t output = (static_cast<int32_t>(raw_wave) * env_level) >> 10;

                op_buffer[op_idx][i] = output; // バッファに書き込み

                // 4. FB履歴更新
                if (is_feedback) {
                    fb_h1 = fb_h0;
                    fb_h0 = output;
                }

                // 状態更新
                op_obj.osc.update(osc_mem);
                op_obj.env.update(env_mem);
            }

            // フィードバック書き戻し
            if (is_feedback) {
                fb_history[n][0] = fb_h0;
                fb_history[n][1] = fb_h1;
            }

            if (!op_obj.env.isFinished(env_mem)) note_is_active = true;
        }

        // --- キャリアのミックス ---
        for(size_t i = 0; i < BUFFER_SIZE; ++i) {
            int32_t sum = 0;
            for(uint8_t k = 0; k < MAX_OPERATORS; ++k) {
                if (output_mask & (1 << k)) {
                    sum += op_buffer[k][i];
                }
            }
            mix_buffer_L[i] += sum;
            mix_buffer_R[i] += sum;
        }

        if(!note_is_active) noteReset(n);
    }

    // 定数キャッシュ
    const int32_t current_scale = master_scale;
    const bool enable_lpf = lpf_enabled;
    const bool enable_hpf = hpf_enabled;
    const bool enable_delay = delay_enabled;
    const int32_t pan_gain_l = AudioMath::PAN_COS_TABLE[master_pan];
    const int32_t pan_gain_r = AudioMath::PAN_SIN_TABLE[master_pan];

    // --- 最終出力段 (Filter, Delay, Pan) ---
    for(size_t i = 0; i < BUFFER_SIZE; ++i) {
        int32_t left = mix_buffer_L[i];
        int32_t right = mix_buffer_R[i];

        // マスターボリューム適用
        left = (left * current_scale) >> 10;
        right = (right * current_scale) >> 10;

        // クリップ処理
        left = AudioMath::fastClampInt16(left);
        right = AudioMath::fastClampInt16(right);
        // フィルタはそれぞれでクリッピングがあるためこれ以降は必要無し

        // エフェクト処理
        // Low-pass filter
        if(enable_lpf) {
            left = filter.processLpfL(left);
            right = filter.processLpfR(right);
        }
        // High-pass filter
        if(enable_hpf) {
            left = filter.processHpfL(left);
            right = filter.processHpfR(right);
        }
        // Delay
        if(enable_delay) {
            left = delay.processL(left);
            right = delay.processR(right);
        }

        // パンニング
        left = (left * pan_gain_l) >> 15;
        right = (right * pan_gain_r) >> 15;

        // 出力バッファへ
        samples_L[i] = static_cast<int16_t>(left);
        samples_R[i] = static_cast<int16_t>(right);

        // バランス接続用反転
        if (left == INT16_MIN) samples_LM[i] = INT16_MAX;
        else samples_LM[i] = static_cast<int16_t>(-left);

        if (right == INT16_MIN) samples_RM[i] = INT16_MAX;
        else samples_RM[i] = static_cast<int16_t>(-right);
    }

    samples_ready_flags = 1;
}

/** @brief シンセ更新 */
FASTRUN void Synth::update() {
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
        noteReset(midi_note_to_index[note]);
    }
    // MAX_NOTES個ノートを演奏中の場合
    if(order_max == MAX_NOTES) {
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
                auto& env_mem = ope_states[op].env_mems[i];
                operators[op].osc.setVelocity(osc_mem, velocity);
                operators[op].osc.setFrequency(osc_mem, note);
                operators[op].osc.setPhase(osc_mem, 0);
                operators[op].env.reset(env_mem); // 初期化IdleからAttackへ
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

    fb_history[index][0] = 0;
    fb_history[index][1] = 0;

    if(order_max > 0) --order_max;
    // 他ノートorder更新
    updateOrder(removed_order);
}

void Synth::reset() {
    for(uint8_t i = 0; i < MAX_NOTES; ++i) {
        noteReset(i);
    }
}

void Synth::setAlgorithm(uint8_t algo_id) {
    current_algo = &Algorithms::get(algo_id);
}

void Synth::setFeedback(uint8_t amount) {
    if (amount > 7) amount = 7;
    feedback_amount = amount;
}