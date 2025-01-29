#include "modules/oscillator.h"

/** @brief Oscillator初期化 */
void Oscillator::init() {
    mod_id = 0;
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

/** @brief oscillatorのphaseを更新 */ //todo mod_mem, mod_env_mem毎回渡すのは手間
void Oscillator::update(Memory& mem, Memory* mod_mem, Envelope::Memory* mod_env_mem) {
    // Modulationあり
    if(mod_osc != nullptr && mod_env != nullptr && mod_mem != nullptr && mod_env_mem != nullptr) {
        // modulationのupdate
        int16_t mod_sample = mod_osc->getSample(*mod_mem) * mod_env->currentLevel(*mod_env_mem) * 0.05f;//todo volume
        float mod_delta = (mod_sample / 32768.0f);
        if(mod_delta < 0.0f) {
            uint32_t modulated_delta = static_cast<uint32_t>(fabs(mod_delta) * UINT32_MAX);
            uint32_t delta_phase = mem.delta - modulated_delta;
            mem.phase += delta_phase;
        }
        else {
            uint32_t modulated_delta = static_cast<uint32_t>(mod_delta * UINT32_MAX);
            uint32_t delta_phase = mem.delta + modulated_delta;
            mem.phase += delta_phase;
        }
        mod_osc->update(*mod_mem);
        mod_env->update(*mod_env_mem);
    // 通常
    } else {
        mem.phase += mem.delta;
    }
}

void Oscillator::setLoopback(bool loopback) {
    this->loopback = loopback;
}

/** @brief oscillatorのサンプルを取得 */
int16_t Oscillator::getSample(Memory& mem) {
    // 波形データからサンプル取得
    size_t index = static_cast<size_t>(mem.phase >> bit_padding) % wavetable_size;
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

void Oscillator::setModulation(uint8_t id, Oscillator* mod_osc, Envelope* mod_env) {
    this->mod_id = id;
    this->mod_osc = mod_osc;
    this->mod_env = mod_env;
}