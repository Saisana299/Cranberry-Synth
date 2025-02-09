#include "modules/synth.hpp"

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

    const uint8_t local_active_count = active_note_count;
    uint8_t local_active_notes[MAX_NOTES];
    for(uint8_t i = 0; i < local_active_count; ++i) {
        local_active_notes[i] = active_notes[i];
    }

    // サンプル毎処理
    for(size_t i = 0; i < BUFFER_SIZE; ++i) {
        // 出力バッファを初期化
        int32_t left = 0;
        int32_t right = 0;

        // ノート毎処理
        for(uint8_t k = 0; k < local_active_count; ++k) {
            uint8_t n = local_active_notes[k];

            // 無効なインデックスはスキップ(本来は不要)
            if(n == INVALID_INDEX) continue;

            // 発音中でなければスキップ(本来は不要？)
            if(notes[n].order == 0) continue;

            uint8_t carrier_cnt = 0;
            uint8_t r_finished_cnt = 0;
            // オペレーター毎処理
            for(uint8_t op = 0; op < MAX_OPERATORS; ++op) {
                // Modulatorの更新処理はCarrier内で行う
                // Carrier出なければスキップ
                if(operators[op].mode != OpMode::Carrier) continue;
                if(!operators[op].osc.isActive()) continue;

                ++carrier_cnt;
                auto& osc_mem = ope_states[op].osc_mems[n];
                auto& env_mem = ope_states[op].env_mems[n];

                // サンプル取得
                // todo ステレオ対応
                int16_t sample = operators[op].osc.getSample(osc_mem, n);

                // エンベロープレベル
                float env_level = operators[op].env.currentLevel(env_mem);

                // 演算順序の固定と乗算回数削減のため
                float scaled_sample = sample * env_level * master_scale;

                // 合計を出力バッファに追加
                // ((サンプル*エンベロープ音量)*調整用レベル)*アンプレベル
                left += static_cast<int32_t>(scaled_sample);
                right += static_cast<int32_t>(scaled_sample);

                // オシレーターとエンベロープを更新
                operators[op].osc.update(osc_mem, n);
                operators[op].env.update(env_mem);

                // Releaseが完了しているか
                // モジュレータ―のエンベロープは考慮しない
                if(operators[op].env.isFinished(env_mem)) {
                    ++r_finished_cnt;
                }
            } // for operator

            // 全てのオペレーターが処理完了
            if(r_finished_cnt == carrier_cnt) {
                resetNote(n);
            }
        } // for active note

        // LPFを適用
        if(lpf_enabled) {
            left = filter.processLpf(left, false);
            right = filter.processLpf(right, true);
        }

        // HPFを適用
        if(hpf_enabled) {
            left = filter.processHpf(left, false);
            right = filter.processHpf(right, true);
        }

        // ディレイを適用
        if(delay_enabled) {
            left = delay.process(left, false);
            right = delay.process(right, true);
        }

        samples_L[i] = std::clamp(left, static_cast<int32_t>(-32768), static_cast<int32_t>(32767));
        samples_R[i] = std::clamp(right, static_cast<int32_t>(-32768), static_cast<int32_t>(32767));
    } // for BUFFER_SIZE

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
            addActiveNote(i);
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
    removeActiveNote(index);
}

/**
 * @brief アクティブノートリストにノートを追加
 */
void Synth::addActiveNote(uint8_t index) {
    // 既に入っていないか確認
    for(uint8_t i = 0; i < active_note_count; ++i) {
        if(active_notes[i] == index) {
            return; // すでに登録済み
        }
    }
    // 空いていれば追加
    if(active_note_count < MAX_NOTES) {
        active_notes[active_note_count++] = index;
    }
}

/**
 * @brief アクティブノートリストからノートを外す
 */
void Synth::removeActiveNote(uint8_t index) {
    for(uint8_t i = 0; i < active_note_count; ++i) {
        if(active_notes[i] == index) {
            // 後ろを詰める
            active_notes[i] = active_notes[active_note_count - 1];
            active_notes[active_note_count - 1] = INVALID_INDEX;
            --active_note_count;
            return;
        }
    }
}