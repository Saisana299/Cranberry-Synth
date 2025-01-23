#include "modules/oscillator.h"

/** @brief Oscillator初期化 */
void Oscillator::init() {
    wavetable = Wavetable::square;
    wavetable_size = sizeof(Wavetable::square) / sizeof(Wavetable::square[0]);
}

/**
 * @brief oscillatorの周波数を設定
 *
 * @param note MIDIノート番号
 */
void Oscillator::setFrequency(Memory& mem, uint8_t note) {
    mem.delta = AudioMath::noteToFrequency(note) / SAMPLE_RATE;
}

/**
 * @brief oscillatorの周波数を設定
 *
 * @param freq 周波数
 */
void Oscillator::setFrequency(Memory& mem, float freq) {
    mem.delta = freq / SAMPLE_RATE;
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
void Oscillator::setPhase(Memory& mem) {
    mem.phase = AudioMath::randomFloat4(0.0f, 1.0f);
    if(mem.phase == 1.0f) mem.phase = 0.0f;
}

/** @brief oscillatorのphaseを更新 */
void Oscillator::update(Memory& mem) {
    mem.phase += mem.delta;
    if(mem.phase >= 1.0f) mem.phase -= 1.0f;
}

/** @brief oscillatorのサンプルを取得 */
int16_t Oscillator::getSample(Memory& mem) {
    size_t index = static_cast<size_t>(mem.phase * wavetable_size) % wavetable_size;
    int16_t sample = wavetable[index];
    return static_cast<int16_t>(sample * mem.vel_vol);
}

void Oscillator::reset(Memory& mem) {
    mem.phase = 0.0f;
    mem.delta = 0.0f;
    mem.vel_vol = 0.0f;
}