#include "modules/oscillator.h"

/** @brief Oscillator初期化 */
void Oscillator::init() {
    phase = 0.0f;
    delta = 0.0f;
    frequency = 0.0f;
}

/**
 * @brief oscillatorの周波数を設定
 *
 * @param note MIDIノート番号
 */
void Oscillator::setFrequency(uint8_t note) {
    frequency = AudioMath::noteToFrequency(note);
    delta = frequency / SAMPLE_RATE;
}

/**
 * @brief oscillatorの周波数を設定
 *
 * @param freq 周波数
 */
void Oscillator::setFrequency(float freq) {
    frequency = freq;
    delta = frequency / SAMPLE_RATE;
}

/** @brief oscillatorのphaseをリセット */
void Oscillator::resetPhase() {
    phase = AudioMath::randomFloat4(0.0f, 1.0f);
    if(phase == 1.0f) phase = 0.0f;
}

/** @brief oscillatorのphaseを更新 */
void Oscillator::update() {
    phase += delta;
    if(phase >= 1.0f) phase -= 1.0f;
}

/** @brief oscillatorのサンプルを取得 */
int16_t Oscillator::getSample() {
    auto& wavetable = Wavetable::square;
    const size_t WAVETABLE_SIZE = sizeof(wavetable) / sizeof(wavetable[0]);
    size_t index = static_cast<size_t>(phase * WAVETABLE_SIZE) % WAVETABLE_SIZE;
    return wavetable[index];
}