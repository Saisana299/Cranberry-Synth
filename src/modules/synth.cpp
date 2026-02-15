#include "modules/synth.hpp"

/** @brief シンセ初期化 */
void Synth::init() {

    // ノート情報クリア
    for (int i = 0; i < 128; ++i) {
        midi_note_to_index[i] = -1;
    }
    Oscillator::initTable();
    lfo_.init();
    loadPreset(0);
}

/** @brief シンセ生成 */
FASTRUN void Synth::generate() {
    if(samples_ready_flags != false) return;
    if(current_algo == nullptr) return;

    // LFOを1バッファ分進める（generate内でバッファ1回保証）
    lfo_.advance(BUFFER_SIZE);

    // 定数キャッシュ
    const uint8_t* exec_order = current_algo->exec_order;
    const uint8_t* mod_mask = current_algo->mod_mask;
    const uint8_t output_mask = current_algo->output_mask;
    const int8_t feedback_op = current_algo->feedback_op;

    // フィードバックシフト計算
    // FEEDBACK_BITDEPTH = 8
    // fb_shift = FEEDBACK_BITDEPTH - feedback (feedback=1→7, feedback=7→1)
    // feedback=0 の場合は16（事実上無効）
    static constexpr uint8_t FEEDBACK_BITDEPTH = 8;
    uint8_t current_fb_shift = (feedback_amount == 0 || feedback_amount > 7)
                               ? 16
                               : (FEEDBACK_BITDEPTH - feedback_amount);

    // フィードバックソースを特定
    // mod_mask[feedback_op] が 0 なら自己フィードバック、そうでなければクロスフィードバック
    int8_t fb_source = feedback_op; // デフォルトは自己フィードバック
    if (feedback_op >= 0 && mod_mask[feedback_op] != 0) {
        // クロスフィードバック：mod_maskからソースオペレーターを特定
        for (uint8_t src = 0; src < MAX_OPERATORS; ++src) {
            if (mod_mask[feedback_op] & (1 << src)) {
                fb_source = src;
                break;
            }
        }
    }

    // 出力バッファをクリア
    Audio24_t mix_buffer_L[BUFFER_SIZE] = {0};
    Audio24_t mix_buffer_R[BUFFER_SIZE] = {0};

    // LFO変調値キャッシュ（バッファ単位で一定）
    const int32_t lfo_pitch_mod = lfo_.getPitchMod();   // 符号付き Q15
    const Gain_t  lfo_amp_mod   = lfo_.getAmpMod();     // [0, Q15_MAX]

    // リセット待ちのノートを記録
    uint8_t notes_to_reset[MAX_NOTES];
    uint8_t reset_count = 0;

    // ノート毎処理
    for(uint8_t n = 0; n < MAX_NOTES; ++n) {
        if(notes[n].order == 0) continue;

        bool note_is_active = false;

        // オペレータ出力の一時保存用バッファ [Op][Sample]
        Audio24_t op_buffer[MAX_OPERATORS][BUFFER_SIZE];

        // フィードバック変数のローカルキャッシュ（ノートループの外でキャッシュ）
        Audio24_t fb_h0 = fb_history[n][0];
        Audio24_t fb_h1 = fb_history[n][1];

        // --- オペレータ毎処理 ---
        // #pragma GCC unroll 6
        for(uint8_t k = 0; k < MAX_OPERATORS; ++k) {
            uint8_t op_idx = exec_order[k];
            Operator& op_obj = operators[op_idx];

            // ステートへの参照キャッシュ
            auto& osc_mem = ope_states[op_idx].osc_mems[n];
            auto& env_mem = ope_states[op_idx].env_mems[n];

            uint8_t mask = mod_mask[op_idx];

            // フィードバック入力を受け取るオペレーター（feedback_op）
            bool is_fb_target = (op_idx == feedback_op && feedback_amount > 0);
            // フィードバック履歴を更新するオペレーター（fb_source）
            bool is_fb_source = (op_idx == fb_source && feedback_amount > 0);

            // エンベロープは64サンプルごとに1回更新
            // BUFFER_SIZE = 128 なので、2回に分けて処理
            constexpr size_t ENV_BLOCK_SIZE = 64;

            // --- 前半64サンプル ---
            // ゲイン補間でクリックノイズを防止
            EnvGain_t gain1 = op_obj.env.currentLevel(env_mem);
            op_obj.env.update(env_mem);
            EnvGain_t gain2 = op_obj.env.currentLevel(env_mem);

            // dgain = (gain2 - gain1 + 32) >> 6 (64サンプルで線形補間)
            int32_t dgain = (static_cast<int32_t>(gain2) - static_cast<int32_t>(gain1) + 32) >> 6;
            int32_t gain = static_cast<int32_t>(gain1);

            for(size_t i = 0; i < ENV_BLOCK_SIZE; ++i) {
                gain += dgain;  // 毎サンプル増分
                Audio24_t mod_input = 0;

                // 1. 変調入力
                if (mask) {
                    for(uint8_t src = 0; src < MAX_OPERATORS; ++src) {
                         if (mask & (1 << src)) {
                             if (is_fb_target && src == fb_source) continue;
                             mod_input += op_buffer[src][i];
                         }
                    }
                }

                // 2. フィードバック入力
                if (is_fb_target && current_fb_shift < 16) {
                    mod_input += (fb_h0 + fb_h1) >> (current_fb_shift + 1);
                }

                // 3. 発音（補間されたゲインを使用）
                Audio24_t raw_wave = op_obj.osc.getSample(osc_mem, mod_input);
                Audio24_t output = Q23_mul_EnvGain(raw_wave, static_cast<EnvGain_t>(gain));

                // 4. LFO 振幅モジュレーション（オペレーター単位）
                if (lfo_amp_mod != 0 && op_ams_gain_[op_idx] != 0) {
                    Gain_t am_amt = static_cast<Gain_t>(
                        (static_cast<int32_t>(lfo_amp_mod) * op_ams_gain_[op_idx]) >> Q15_SHIFT
                    );
                    output -= static_cast<Audio24_t>(
                        (static_cast<int64_t>(output) * am_amt) >> Q15_SHIFT
                    );
                }

                op_buffer[op_idx][i] = output;

                // 5. FB履歴更新
                if (is_fb_source) {
                    fb_h1 = fb_h0;
                    fb_h0 = output;
                }

                op_obj.osc.update(osc_mem);

                // 6. LFO ピッチモジュレーション（位相オフセット追加）
                if (lfo_pitch_mod != 0) {
                    osc_mem.phase += static_cast<Phase_t>(
                        (static_cast<int64_t>(osc_mem.delta) * lfo_pitch_mod) >> Q15_SHIFT
                    );
                }
            }

            // --- 後半64サンプル ---
            // 前半終了時のgain2を次のgain1として使用
            gain1 = gain2;
            op_obj.env.update(env_mem);
            gain2 = op_obj.env.currentLevel(env_mem);

            dgain = (static_cast<int32_t>(gain2) - static_cast<int32_t>(gain1) + 32) >> 6;
            gain = static_cast<int32_t>(gain1);

            for(size_t i = ENV_BLOCK_SIZE; i < BUFFER_SIZE; ++i) {
                gain += dgain;  // 毎サンプル増分
                Audio24_t mod_input = 0;

                // 1. 変調入力
                if (mask) {
                    for(uint8_t src = 0; src < MAX_OPERATORS; ++src) {
                         if (mask & (1 << src)) {
                             if (is_fb_target && src == fb_source) continue;
                             mod_input += op_buffer[src][i];
                         }
                    }
                }

                // 2. フィードバック入力
                if (is_fb_target && current_fb_shift < 16) {
                    mod_input += (fb_h0 + fb_h1) >> (current_fb_shift + 1);
                }

                // 3. 発音（補間されたゲインを使用）
                Audio24_t raw_wave = op_obj.osc.getSample(osc_mem, mod_input);
                Audio24_t output = Q23_mul_EnvGain(raw_wave, static_cast<EnvGain_t>(gain));

                // 4. LFO 振幅モジュレーション（オペレーター単位）
                if (lfo_amp_mod != 0 && op_ams_gain_[op_idx] != 0) {
                    Gain_t am_amt = static_cast<Gain_t>(
                        (static_cast<int32_t>(lfo_amp_mod) * op_ams_gain_[op_idx]) >> Q15_SHIFT
                    );
                    output -= static_cast<Audio24_t>(
                        (static_cast<int64_t>(output) * am_amt) >> Q15_SHIFT
                    );
                }

                op_buffer[op_idx][i] = output;

                // 5. FB履歴更新
                if (is_fb_source) {
                    fb_h1 = fb_h0;
                    fb_h0 = output;
                }

                op_obj.osc.update(osc_mem);

                // 6. LFO ピッチモジュレーション（位相オフセット追加）
                if (lfo_pitch_mod != 0) {
                    osc_mem.phase += static_cast<Phase_t>(
                        (static_cast<int64_t>(osc_mem.delta) * lfo_pitch_mod) >> Q15_SHIFT
                    );
                }
            }

            // キャリアのみでノートアクティブ判定（モジュレーターは無視）
            // モジュレーターが終わっていなくても、キャリアが終われば音は出ない
            if ((output_mask & (1 << op_idx)) && !op_obj.env.isFinished(env_mem)) {
                note_is_active = true;
            }
        }

        // フィードバック書き戻し
        fb_history[n][0] = fb_h0;
        fb_history[n][1] = fb_h1;

        // --- キャリアのミックス ---
        Audio24_t max_output = 0;  // このノートの最大出力レベルを記録
        for(size_t i = 0; i < BUFFER_SIZE; ++i) {
            Audio24_t sum = 0;
            for(uint8_t k = 0; k < MAX_OPERATORS; ++k) {
                if (output_mask & (1 << k)) {
                    sum += op_buffer[k][i];
                }
            }

            // NOTE: LFO AMはオペレーター単位で適用済み

            mix_buffer_L[i] += sum;
            mix_buffer_R[i] += sum;

            // 最大出力レベルを更新（絶対値で比較）
            Audio24_t abs_sum = (sum >= 0) ? sum : -sum;
            if (abs_sum > max_output) max_output = abs_sum;
        }

        // 最終出力が無音に近い場合（閾値: 16）は終了とみなす
        // ただし、リリース中（Phase4）またはIdle状態のときのみ
        // アタック/ディケイ/サステイン中は出力が小さくてもリセットしない
        bool is_in_release = true;
        for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
            if (output_mask & (1 << op)) {  // キャリアのみチェック
                auto state = ope_states[op].env_mems[n].state;
                if (state != Envelope::EnvelopeState::Phase4 &&
                    state != Envelope::EnvelopeState::Idle) {
                    is_in_release = false;
                    break;
                }
            }
        }

        if(!note_is_active || (is_in_release && max_output < 16)) {
            notes_to_reset[reset_count++] = n;
        }
    }

    // ループ終了後にまとめてリセット
    for(uint8_t r = 0; r < reset_count; ++r) {
        noteReset(notes_to_reset[r]);
    }

    // 定数キャッシュ
    const Gain_t current_scale = output_scale;
    const bool enable_lpf = lpf_enabled;
    const bool enable_hpf = hpf_enabled;
    const bool enable_delay = delay_enabled;
    // const int32_t pan_gain_l = AudioMath::PAN_COS_TABLE[master_pan];
    // const int32_t pan_gain_r = AudioMath::PAN_SIN_TABLE[master_pan];

    // --- 最終出力段 (Filter, Delay) ---
    for(size_t i = 0; i < BUFFER_SIZE; ++i) {
        Audio24_t left = mix_buffer_L[i];
        Audio24_t right = mix_buffer_R[i];

        // マスターボリューム適用 (Q23 × Q15 → Q23)
        left = Q23_mul_Q15(left, current_scale);
        right = Q23_mul_Q15(right, current_scale);

        // Q23 → 16bit 変換（フィルター/ディレイは16bitで処理）
        Sample16_t left_16 = Q23_to_Sample16(left);
        Sample16_t right_16 = Q23_to_Sample16(right);

        // エフェクト処理 (16bit)
        // Low-pass filter
        if(enable_lpf) {
            left_16 = filter.processLpfL(left_16);
            right_16 = filter.processLpfR(right_16);
        }
        // High-pass filter
        if(enable_hpf) {
            left_16 = filter.processHpfL(left_16);
            right_16 = filter.processHpfR(right_16);
        }
        // Delay
        if(enable_delay) {
            left_16 = delay.processL(left_16);
            right_16 = delay.processR(right_16);
        }

        // 出力バッファへ
        samples_L[i] = left_16;
        samples_R[i] = right_16;

        // バランス接続用反転
        if (left_16 == SAMPLE16_MIN) samples_LM[i] = SAMPLE16_MAX;
        else samples_LM[i] = static_cast<Sample16_t>(-left_16);

        if (right_16 == SAMPLE16_MIN) samples_RM[i] = SAMPLE16_MAX;
        else samples_RM[i] = static_cast<Sample16_t>(-right_16);
    }

    samples_ready_flags = true;
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
    uint8_t active_count = 0;
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if (notes[i].order > removed) {
            --notes[i].order;
        }
        if (notes[i].order > 0) {
            ++active_count;
        }
    }
    order_max = active_count;
}

/**
 * @brief 演奏するノートを追加します
 *
 * @param note MIDIノート番号
 * @param velocity MIDIベロシティ
 * @param channel MIDIチャンネル
 */
void Synth::noteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    // LFO KEY SYNC——ノートオン毎にディレイカウンターをリセット（key_sync_ trueなら位相も）
    lfo_.keyOn();

    // トランスポーズを適用
    int16_t transposed_note = note + transpose;
    if (transposed_note < 0) transposed_note = 0;
    if (transposed_note > 127) transposed_note = 127;
    uint8_t actual_note = static_cast<uint8_t>(transposed_note);

    // 既に同じノートを演奏している場合は弾き直し（リトリガー）
    if(midi_note_to_index[note] != -1) {
        uint8_t i = midi_note_to_index[note];
        uint8_t old_order = notes[i].order;
        notes[i].velocity = velocity;
        // orderを最新に更新（他のノートのorderを詰める）
        for (uint8_t j = 0; j < MAX_NOTES; ++j) {
            if (notes[j].order > old_order) {
                --notes[j].order;
            }
        }
        notes[i].order = order_max;  // 最新のorderを割り当て
        for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
            auto& osc_mem = ope_states[op].osc_mems[i];
            auto& env_mem = ope_states[op].env_mems[i];
            operators[op].osc.setFrequency(osc_mem, actual_note);
            // Output Level + Velocity + Keyboard Level Scaling をエンベロープのoutlevelとして設定
            uint8_t op_level = operators[op].osc.getLevel();
            operators[op].env.setOutlevel(op_level, velocity, actual_note, operators[op].env.getVelocitySens());
            operators[op].env.calcNoteTargetLevels(env_mem); // ノートごとのターゲットレベル計算
            operators[op].env.applyRateScaling(env_mem, actual_note); // Rate Scaling適用
            operators[op].env.reset(env_mem); // エンベロープをAttackから再開
        }
        return;
    }

    // MAX_NOTES個ノートを演奏中の場合、スティール対象を探す
    // 優先順位: 1. リリース中で最古のノート  2. 全体で最古のノート
    if(order_max >= MAX_NOTES) {
        int8_t releasing_oldest_index = -1;
        uint8_t releasing_oldest_order = 255;

        // リリース中のノートで最古のものを探す
        for (uint8_t i = 0; i < MAX_NOTES; ++i) {
            if (notes[i].order == 0) continue;

            // 全オペレーターがPhase4 (Release) or Idleかチェック（noteOff済み）
            bool is_releasing = true;
            for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
                auto& env_mem = ope_states[op].env_mems[i];
                auto state = env_mem.state;
                if (state != Envelope::EnvelopeState::Phase4 &&
                    state != Envelope::EnvelopeState::Idle) {
                    is_releasing = false;
                    break;
                }
            }

            // リリース中で、より古いノートなら記録
            if (is_releasing && notes[i].order < releasing_oldest_order) {
                releasing_oldest_order = notes[i].order;
                releasing_oldest_index = i;
            }
        }

        // リリース中のノートがあればそれを、なければ最古(order==1)をリセット
        if (releasing_oldest_index >= 0) {
            noteReset(releasing_oldest_index);
        } else {
            // order == 1（最古）のノートを探す
            for (uint8_t i = 0; i < MAX_NOTES; ++i) {
                if (notes[i].order == 1) {
                    noteReset(i);
                    break;
                }
            }
        }
    }

    // ノートを追加する
    for (uint8_t i = 0; i < MAX_NOTES; ++i) {
        if (notes[i].order == 0) {
            midi_note_to_index[note] = i;
            auto& it = notes[i];
            ++order_max;
            it.order = order_max;  // updateOrderで詰められるので、order_maxと一致
            it.note = note;
            it.velocity = velocity;
            it.channel = channel;

            for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
                auto& osc_mem = ope_states[op].osc_mems[i];
                auto& env_mem = ope_states[op].env_mems[i];
                operators[op].osc.setFrequency(osc_mem, actual_note);
                if (osc_key_sync_) {
                    operators[op].osc.setPhase(osc_mem, 0);
                }
                // Output Level + Velocity + Keyboard Level Scaling をエンベロープのoutlevelとして設定
                uint8_t op_level = operators[op].osc.getLevel();
                operators[op].env.setOutlevel(op_level, velocity, actual_note, operators[op].env.getVelocitySens());
                operators[op].env.calcNoteTargetLevels(env_mem); // ノートごとのターゲットレベル計算
                operators[op].env.applyRateScaling(env_mem, actual_note); // Rate Scaling適用
                operators[op].env.reset(env_mem); // 初期化IdleからAttackへ
            }
            return;
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
    // 既にリセット済みの場合は何もしない
    if(it.order == 0) return;

    uint8_t removed_order = it.order;
    // 有効なノート番号の場合のみマッピングをクリア（配列外アクセス防止）
    if(it.note < 128) {
        midi_note_to_index[it.note] = -1;
    }
    it.order = 0;
    it.note = 255;
    it.velocity = 0;
    it.channel = 0;
    for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
        auto& oper = operators[op];
        auto& osc_mem = ope_states[op].osc_mems[index];
        auto& env_mem = ope_states[op].env_mems[index];
        oper.osc.reset(osc_mem);
        oper.env.clear(env_mem);  // Idle状態に完全リセット
    }

    fb_history[index][0] = 0;
    fb_history[index][1] = 0;

    // 他ノートorder更新（order_maxも一緒に更新される）
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

void Synth::loadPreset(uint8_t preset_id) {
    // プリセットを取得
    const SynthPreset& preset = DefaultPresets::get(preset_id);

    // 現在のプリセットIDを保存
    current_preset_id = preset_id;

    // アルゴリズムとフィードバックを設定
    setAlgorithm(preset.algorithm_id);
    setFeedback(preset.feedback);

    // 各オペレーターの設定を適用
    active_carriers = 0; // メンバ変数をリセット
    for (uint8_t i = 0; i < MAX_OPERATORS; ++i) {
        const OperatorPreset& op_preset = preset.operators[i];

        if (op_preset.enabled) {
            // オシレーター設定
            operators[i].osc.setWavetable(op_preset.wavetable_id);
            operators[i].osc.setLevelNonLinear(op_preset.level);
            operators[i].osc.setCoarse(op_preset.coarse);
            operators[i].osc.setFine(op_preset.fine);
            operators[i].osc.setDetune(op_preset.detune);
            operators[i].osc.setFixed(op_preset.is_fixed);
            operators[i].osc.enable();

            // エンベロープ設定 (Rate/Level)
            operators[i].env.setRate1(op_preset.rate1);
            operators[i].env.setRate2(op_preset.rate2);
            operators[i].env.setRate3(op_preset.rate3);
            operators[i].env.setRate4(op_preset.rate4);
            operators[i].env.setLevel1(op_preset.level1);
            operators[i].env.setLevel2(op_preset.level2);
            operators[i].env.setLevel3(op_preset.level3);
            operators[i].env.setLevel4(op_preset.level4);
            operators[i].env.setRateScaling(op_preset.rate_scaling);

            // Keyboard Level Scaling設定
            operators[i].env.setBreakPoint(op_preset.kbd_break_point);
            operators[i].env.setLeftDepth(op_preset.kbd_left_depth);
            operators[i].env.setRightDepth(op_preset.kbd_right_depth);
            operators[i].env.setLeftCurve(op_preset.kbd_left_curve);
            operators[i].env.setRightCurve(op_preset.kbd_right_curve);

            // ベロシティ感度設定
            operators[i].env.setVelocitySens(op_preset.velocity_sens);

            // AMS感度設定 (0-3)
            op_ams_gain_[i] = Lfo::AMS_TAB[op_preset.amp_mod_sens & 3];

            // キャリアの数をカウント
            if (current_algo && (current_algo->output_mask & (1 << i))) {
                active_carriers++;
            }
        } else {
            // オペレーター無効化
            operators[i].osc.disable();
            op_ams_gain_[i] = 0;
        }
    }

    // エフェクト設定を適用
    const EffectPreset& fx = preset.effects;

    // ディレイ設定
    delay_enabled = fx.delay_enabled;
    delay.setDelay(fx.delay_time, fx.delay_level, fx.delay_feedback);

    // ローパスフィルタ設定
    lpf_enabled = fx.lpf_enabled;
    filter.setLowPass(fx.lpf_cutoff, fx.lpf_resonance);
    filter.setLpfMix(fx.lpf_mix);

    // ハイパスフィルタ設定
    hpf_enabled = fx.hpf_enabled;
    filter.setHighPass(fx.hpf_cutoff, fx.hpf_resonance);
    filter.setHpfMix(fx.hpf_mix);

    // LFO設定を適用
    const LfoPreset& lfo_p = preset.lfo;
    lfo_.setWave(lfo_p.wave);
    lfo_.setSpeed(lfo_p.speed);
    lfo_.setDelay(lfo_p.delay);
    lfo_.setPmDepth(lfo_p.pm_depth);
    lfo_.setAmDepth(lfo_p.am_depth);
    lfo_.setPitchModSens(lfo_p.pitch_mod_sens);
    lfo_.setKeySync(lfo_p.key_sync);
    osc_key_sync_ = lfo_p.osc_key_sync;
    lfo_.reset();

    // マスタースケールを調整
    if (active_carriers == 0) active_carriers = 1; // 0除算防止
    output_scale = static_cast<Gain_t>((static_cast<int32_t>(master_volume / active_carriers) * polyphony_divisor) >> Q15_SHIFT);
}

const char* Synth::getCurrentPresetName() const {
    return DefaultPresets::get(current_preset_id).name;
}

uint8_t Synth::getCurrentAlgorithmId() const {
    // current_algoからアルゴリズムIDを逆引き
    // 簡易的な実装: Algorithms::get()と比較
    for (uint8_t i = 0; i < 32; ++i) {
        if (&Algorithms::get(i) == current_algo) {
            return i;
        }
    }
    return 0; // デフォルト
}
