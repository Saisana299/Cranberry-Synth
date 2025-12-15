#include "modules/synth.hpp"

/** @brief シンセ初期化 */
void Synth::init() {

    // ノート情報クリア
    for (int i = 0; i < 128; ++i) {
        midi_note_to_index[i] = -1;
    }
    Oscillator::initTable();

    // --- ペア1: Op1(Mod) -> Op0(Car) ---
    // Carrier: Op0
    operators[0].mode = OpMode::Carrier;
    operators[0].osc.setLevel(1024);
    operators[0].osc.enable();
    operators[0].osc.setDetune(3);
    operators[0].env.setDecay(60);
    operators[0].env.setSustain(0);
    operators[0].env.setRelease(80);

    // Modulator: Op1
    operators[1].mode = OpMode::Modulator;
    operators[1].osc.setLevelNonLinear(35);
    operators[1].osc.enable();
    operators[1].osc.setCoarse(14.0f);
    operators[1].env.setDecay(60);
    operators[1].env.setSustain(0);

    // --- ペア2: Op3(Mod) -> Op2(Car) ---
    // Carrier: Op2
    operators[2].mode = OpMode::Carrier;
    operators[2].osc.setLevel(1024);
    operators[2].osc.enable();
    operators[2].env.setDecay(94);
    operators[2].env.setSustain(0);
    operators[2].env.setRelease(80);

    // Modulator: Op3
    operators[3].mode = OpMode::Modulator;
    operators[3].osc.setLevelNonLinear(89);
    operators[3].osc.enable();
    operators[3].env.setDecay(94);
    operators[3].env.setSustain(0);
    operators[3].env.setRelease(80);

    // --- ペア3: Op5(Mod) -> Op4(Car) ---
    // Carrier: Op4
    operators[4].mode = OpMode::Carrier;
    operators[4].osc.setLevel(1024);
    operators[4].osc.enable();
    operators[4].osc.setDetune(-7);
    operators[4].env.setDecay(94);
    operators[4].env.setSustain(0);
    operators[4].env.setRelease(80);

    // Modulator: Op5
    operators[5].mode = OpMode::Modulator;
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

        // ノート毎処理
        for(uint8_t n = 0; n < MAX_NOTES; ++n) {
            // 発音中でなければスキップ
            if(notes[n].order == 0) continue;

            // キャリアのエンベロープが全て終わっているか判定
            bool note_is_active = false;

            // --- Pair 1: Op1 (Mod) -> Op0 (Car) ---
            {
                // 1. Modulator (Op1)
                Operator& mod = operators[1];
                auto& mod_mem = ope_states[1].osc_mems[n];
                auto& mod_env_mem = ope_states[1].env_mems[n];

                // モジュレーターは変調なし(0)で波形取得
                int16_t mod_raw = mod.osc.getSample(mod_mem, 0);
                int32_t mod_env_lvl = mod.env.currentLevel(mod_env_mem);
                // 変調量を計算 (波形 * エンベロープ)
                int32_t mod_product = (static_cast<int32_t>(mod_raw) * mod_env_lvl) >> 10;

                mod.osc.update(mod_mem);
                mod.env.update(mod_env_mem);

                // 2. Carrier (Op0)
                Operator& car = operators[0];
                auto& car_mem = ope_states[0].osc_mems[n];
                auto& car_env_mem = ope_states[0].env_mems[n];

                // モジュレーターの出力(mod_product)を渡す
                int16_t car_raw = car.osc.getSample(car_mem, mod_product);
                int32_t car_env_lvl = car.env.currentLevel(car_env_mem);
                int32_t car_out = (static_cast<int32_t>(car_raw) * car_env_lvl) >> 10;

                // 出力加算
                left += car_out;
                right += car_out;

                car.osc.update(car_mem);
                car.env.update(car_env_mem);

                // キャリアが終わっていなければ生存フラグON
                if (!car.env.isFinished(car_env_mem)) note_is_active = true;
            }

            // --- Pair 2: Op3 (Mod) -> Op2 (Car) ---
            {
                // Modulator (Op3)
                Operator& mod = operators[3];
                auto& mod_mem = ope_states[3].osc_mems[n];
                auto& mod_env_mem = ope_states[3].env_mems[n];

                int16_t mod_raw = mod.osc.getSample(mod_mem, 0);
                int32_t mod_env_lvl = mod.env.currentLevel(mod_env_mem);
                int32_t mod_product = (static_cast<int32_t>(mod_raw) * mod_env_lvl) >> 10;

                mod.osc.update(mod_mem);
                mod.env.update(mod_env_mem);

                // Carrier (Op2)
                Operator& car = operators[2];
                auto& car_mem = ope_states[2].osc_mems[n];
                auto& car_env_mem = ope_states[2].env_mems[n];

                // 変調入力
                int16_t car_raw = car.osc.getSample(car_mem, mod_product);
                int32_t car_env_lvl = car.env.currentLevel(car_env_mem);
                int32_t car_out = (static_cast<int32_t>(car_raw) * car_env_lvl) >> 10;

                left += car_out;
                right += car_out;

                car.osc.update(car_mem);
                car.env.update(car_env_mem);

                if (!car.env.isFinished(car_env_mem)) note_is_active = true;
            }

            // --- Pair 3: Op5 (Mod) -> Op4 (Car) ---
            {
                // Modulator (Op5)
                Operator& mod = operators[5];
                auto& mod_mem = ope_states[5].osc_mems[n];
                auto& mod_env_mem = ope_states[5].env_mems[n];

                int16_t mod_raw = mod.osc.getSample(mod_mem, 0);
                int32_t mod_env_lvl = mod.env.currentLevel(mod_env_mem);
                int32_t mod_product = (static_cast<int32_t>(mod_raw) * mod_env_lvl) >> 10;

                mod.osc.update(mod_mem);
                mod.env.update(mod_env_mem);

                // Carrier (Op4)
                Operator& car = operators[4];
                auto& car_mem = ope_states[4].osc_mems[n];
                auto& car_env_mem = ope_states[4].env_mems[n];

                // 変調入力
                int16_t car_raw = car.osc.getSample(car_mem, mod_product);
                int32_t car_env_lvl = car.env.currentLevel(car_env_mem);
                int32_t car_out = (static_cast<int32_t>(car_raw) * car_env_lvl) >> 10;

                left += car_out;
                right += car_out;

                car.osc.update(car_mem);
                car.env.update(car_env_mem);

                if (!car.env.isFinished(car_env_mem)) note_is_active = true;
            }

            // 全てのキャリアが終了していたらノートをリセット
            if(!note_is_active) {
                noteReset(n);
            }

        } // for active note

        // マスターボリューム適用
        left = (left * MASTER_SCALE) >> 10;
        right = (right * MASTER_SCALE) >> 10;

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
        if(left == INT16_MIN) samples_LM[i] = INT16_MAX;
        else samples_LM[i] = static_cast<int16_t>(-left);

        if(right == INT16_MIN) samples_RM[i] = INT16_MAX;
        else samples_RM[i] = static_cast<int16_t>(-right);

    } // for BUFFER_SIZE

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
        // 必要に応じてリトリガー処理
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
    if(order_max > 0) --order_max;
    // 他ノートorder更新
    updateOrder(removed_order);
}

void Synth::reset() {
    for(uint8_t i = 0; i < MAX_NOTES; ++i) {
        noteReset(i);
    }
}