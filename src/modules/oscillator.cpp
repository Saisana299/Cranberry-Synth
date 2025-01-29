#include "modules/oscillator.h"

/** @brief Oscillator初期化 */
void Oscillator::init() {
    mod_osc = nullptr;
    mod_env = nullptr;
    loopback = false;
    enabled = false;
    wavetable = Wavetable::sine;
    wavetable_size = sizeof(Wavetable::sine) / sizeof(Wavetable::sine[0]);
    bit_padding = AudioMath::bitPadding32(wavetable_size);
}

/**
 * @brief oscillatorの周波数を設定
 *
 * @param note MIDIノート番号
 */
void Oscillator::setFrequency(Memory& mem, uint8_t note) {
    mem.delta = AudioMath::noteToFrequency(note) * F_1ULL32 / SAMPLE_RATE;
}

/**
 * @brief oscillatorの周波数を設定
 *
 * @param freq 周波数
 */
void Oscillator::setFrequency(Memory& mem, float freq) {
    mem.delta = freq * F_1ULL32 / SAMPLE_RATE;
}

/**
 * @brief 
 * 
 * @param mem 
 * @param velocity 
 */
void Oscillator::setVolume(Memory& mem, uint8_t velocity) {
    mem.vel_vol = AudioMath::velocityToAmplitude(velocity);
}

/** @brief oscillatorのphaseをリセット */
void Oscillator::setPhase(Memory& mem, uint32_t phase) {
    mem.phase = phase;//todo
}

/** @brief oscillatorのphaseを更新 */
void Oscillator::update(Memory& mem, uint8_t note_id) {
    // 通常の処理
    mem.phase += mem.delta;

    // Modulationあり
    if(mod_osc != nullptr) {
        // nullチェック
        if(mod_env == nullptr || mod_osc_mems == nullptr || mod_env_mems == nullptr) return;

        //------ 危険地帯：mod_osc_memsとmod_env_memsの要素数がnote_id以上という前提 ------//

        // キャッシュ
        Oscillator::Memory& mod_mem = mod_osc_mems[note_id];
        Envelope::Memory& mod_env_mem = mod_env_mems[note_id];

        mod_osc->update(mod_mem, note_id);
        mod_env->update(mod_env_mem);

        // ----------------------------------------------------------------------------- //
    }
}

void Oscillator::setLoopback(bool loopback) {
    this->loopback = loopback;
}

/** @brief oscillatorのサンプルを取得 */
int16_t Oscillator::getSample(Memory& mem, uint8_t note_id) {
    // キャリアのベース位相
    uint32_t base_phase = mem.phase;

    // モジュレーションがある場合
    if(mod_osc != nullptr) {
        if(mod_env != nullptr && mod_osc_mems != nullptr && mod_env_mems != nullptr) {
            //------ 危険地帯：mod_osc_memsとmod_env_memsの要素数がnote_id以上という前提 ------//

            // キャッシュ
            Oscillator::Memory& mod_mem = mod_osc_mems[note_id];
            Envelope::Memory& mod_env_mem = mod_env_mems[note_id];

            // mod_osc->getSample(mod_mem) は -32768~32767 の範囲を想定
            float mod_env_level = mod_env->currentLevel(mod_env_mem);
            float mod_sample_f = (mod_osc->getSample(mod_mem, note_id) / 32768.0f) * mod_env_level * 1.8f;

            // mod_sample_f の値を位相単位にスケーリングして加算
            uint32_t mod_phase_offset = static_cast<uint32_t>(mod_sample_f * (float)0xFFFFFFFFu);

            // base_phase に加算
            if(mod_sample_f >= 0.0f) {
                base_phase += mod_phase_offset;
            } else {
                base_phase -= mod_phase_offset;
            }

            // ----------------------------------------------------------------------------- //
        }
    }

    // 波形テーブルを参照する
    size_t index = static_cast<size_t>(base_phase >> bit_padding) % wavetable_size;
    int16_t sample = wavetable[index];

    // ベロシティを適用
    return static_cast<int16_t>(sample * mem.vel_vol);
}

void Oscillator::enable() {
    enabled = true;
}

void Oscillator::disable() {
    enabled = false;
}

bool Oscillator::isActive() {
    return enabled;
}

void Oscillator::reset(Memory& mem) {
    mem.phase = 0;
    mem.delta = 0;
    mem.vel_vol = 0.0f;
}

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