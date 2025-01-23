#include "modules/oscillator.h"

/** @brief Oscillator初期化 */
void Oscillator::init() {
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

/** @brief oscillatorのphaseをリセット */
void Oscillator::resetPhase(Memory& mem) {
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
    auto& wavetable = Wavetable::square;
    const size_t WAVETABLE_SIZE = sizeof(wavetable) / sizeof(wavetable[0]);
    size_t index = static_cast<size_t>(mem.phase * WAVETABLE_SIZE) % WAVETABLE_SIZE;
    return wavetable[index];
}