#include "modules/oscillator.hpp"

/**
 * @brief oscillatorの周波数を設定
 *
 * @param note MIDIノート番号
 */
void Oscillator::setFrequency(Memory& mem, uint8_t note) {
    mem.delta = AudioMath::ratioToFrequency(note, detune_cents, coarse, fine_level) * F_1ULL32 / SAMPLE_RATE;
}

/**
 * @brief オシレーターのベロシティボリュームを設定
 *
 * @param velocity ベロシティ値
 */
void Oscillator::setVelocity(Memory& mem, uint8_t velocity) {
    mem.vel_vol = AudioMath::velocityToAmplitude(velocity);
}

/**
 * @brief oscillatorの位相をリセット
 *
 * @param phase 位相
*/
void Oscillator::setPhase(Memory& mem, uint32_t phase) {
    mem.phase = phase;
}

/**
 * @brief オシレーターの状態を更新
 *
 * @param note_id ノートID
*/
void Oscillator::update(Memory& mem, uint8_t note_id) {
    if(!enabled) return;

    // 通常の処理
    mem.phase += mem.delta;

    // Modulationあり
    if (mod_osc && mod_env && mod_osc_mems && mod_env_mems) {
        //------ 危険地帯：mod_osc_memsとmod_env_memsの要素数がnote_id以上という前提 ------//

        // キャッシュ
        Oscillator::Memory& mod_mem = mod_osc_mems[note_id];
        Envelope::Memory& mod_env_mem = mod_env_mems[note_id];

        mod_osc->update(mod_mem, note_id);
        mod_env->update(mod_env_mem);

        // ----------------------------------------------------------------------------- //
    }
}

/** @brief feedbackを設定 */
void Oscillator::setFeedback(bool is_feedback) {
    this->is_feedback = is_feedback;
}

/**
 * @brief oscillatorのサンプルを取得
 *
 * @param note_id ノートID
 */
int16_t Oscillator::getSample(Memory& mem, uint8_t note_id) {
    if(!enabled) return 0;

    // ローカル変数にキャッシュ
    const float local_vel_vol = mem.vel_vol;
    const uint32_t local_phase = mem.phase;

    // キャリアのベース位相
    uint32_t base_phase = local_phase;

    // モジュレーションがある場合
    if(mod_osc && mod_env && mod_osc_mems && mod_env_mems) {
        //------ 危険地帯：mod_osc_memsとmod_env_memsの要素数がnote_id以上という前提 ------//

        // ローカルキャッシュ
        Oscillator* local_mod_osc = mod_osc;
        Envelope*   local_mod_env = mod_env;
        Oscillator::Memory* local_mod_osc_mems = mod_osc_mems;
        Envelope::Memory*   local_mod_env_mems = mod_env_mems;

        // モジュレーション用メモリの取得
        Oscillator::Memory& mod_mem = local_mod_osc_mems[note_id];
        Envelope::Memory&   mod_env_mem = local_mod_env_mems[note_id];

        // モジュレーターエンベロープのレベル
        const float mod_env_level = local_mod_env->currentLevel(mod_env_mem);
        // モジュレーターのサンプルを取得
        const float mod_sample = static_cast<float>(local_mod_osc->getSample(mod_mem, note_id));
        // モジュレーション比率を計算
        const float mod_sample_ratio = mod_sample * mod_env_level * INV_INT16_MAXf;
        // モジュレーションの位相オフセット
        const uint32_t mod_phase_offset = static_cast<uint32_t>(fabsf(mod_sample_ratio) * PHASE_RANGE_F);

        // base_phase に加減算
        base_phase += (mod_sample_ratio < 0) ? -mod_phase_offset : mod_phase_offset;

        // ----------------------------------------------------------------------------- //
    }

    // 波形テーブルを参照する
    const size_t index = (base_phase >> bit_padding) & (wavetable_size - 1); // サンプル数が2^Nである前提
    const int16_t sample = wavetable[index];

    // ベロシティレベルとオシレーターレベルを適用
    return static_cast<int16_t>(sample * (local_vel_vol * level));
}

/** @brief オシレーターを有効化 */
void Oscillator::enable() {
    enabled = true;
}

/** @brief オシレーターを無効化 */
void Oscillator::disable() {
    enabled = false;
}

/** @brief オシレーターの状態 */
bool Oscillator::isActive() {
    return enabled;
}

/** @brief オシレーターの状態をリセット */
void Oscillator::reset(Memory& mem) {
    mem.phase = 0;
    mem.delta = 0;
    mem.vel_vol = 0.0f;
}

/**
 * @brief オシレーターのモジュレーションを設定
 *
 * @param mod_osc モジュレーターオシレーター
 * @param mod_env モジュレーターエンベロープ
 * @param mod_osc_mems モジュレーターオシレーターのメモリ(ノートごと)
 * @param mod_env_mems モジュレーターエンベロープのメモリ(ノートごと)
 */
void Oscillator::setModulation(
    Oscillator* mod_osc,
    Envelope* mod_env,
    Oscillator::Memory* mod_osc_mems,
    Envelope::Memory* mod_env_mems
) {
    this->mod_osc = mod_osc;
    this->mod_env = mod_env;
    this->mod_osc_mems = mod_osc_mems;
    this->mod_env_mems = mod_env_mems;
}

/**
 * @brief オシレーターの音量を設定
 *
 * @param level 0.0~1.0
 */
void Oscillator::setLevel(float level) {
    if(level < 0.0f || level > 1.0f) return;
    this->level = level;
}

/**
 * @brief 波形テーブルを設定
 *
 * @param table_id id
 */
void Oscillator::setWavetable(uint8_t table_id) {
    switch(table_id) {
    case 0:
        wavetable = Wavetable::sine;
        wavetable_size = sizeof(Wavetable::sine) / sizeof(Wavetable::sine[0]);
        break;
    case 1:
        wavetable = Wavetable::triangle;
        wavetable_size = sizeof(Wavetable::triangle) / sizeof(Wavetable::triangle[0]);
        break;
    case 2:
        wavetable = Wavetable::saw;
        wavetable_size = sizeof(Wavetable::saw) / sizeof(Wavetable::saw[0]);
        break;
    case 3:
        wavetable = Wavetable::square;
        wavetable_size = sizeof(Wavetable::square) / sizeof(Wavetable::square[0]);
        break;
    }
}

void Oscillator::setCoarse(float coarse) {
    // todo条件式
    this->coarse = coarse;
}

void Oscillator::setFine(float fine_level) {
    // todo条件式
    this->fine_level = fine_level;
}

void Oscillator::setDetune(int8_t detune_cents) {
    // todo条件式
    this->detune_cents = detune_cents;
}

void Oscillator::setLevelNonLinear(uint8_t level) {
    float x = level / 100.0f;
    const float exponent = std::log(0.5f) / std::log(0.75f);
    this->level = std::pow(x, exponent);
}