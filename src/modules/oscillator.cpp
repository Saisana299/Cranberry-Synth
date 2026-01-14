#include "modules/oscillator.hpp"

int16_t Oscillator::level_table[100] = {};
bool Oscillator::table_initialized = false;

/**
 * @brief oscillatorの周波数を設定
 *
 * @param mem オシレーターメモリ
 * @param note MIDIノート番号
 */
void Oscillator::setFrequency(Memory& mem, uint8_t note) {
    float freq;
    if (is_fixed) {
        // FIXEDモード: MIDIノートに関係なく固定周波数
        freq = AudioMath::fixedToFrequency(detune_cents, coarse, fine_level);
    } else {
        // RATIOモード: MIDIノートに対する比率で周波数を設定
        freq = AudioMath::ratioToFrequency(note, detune_cents, coarse, fine_level);
    }
    mem.delta = static_cast<uint32_t>(freq * PHASE_SCALE_FACTOR);
}

/**
 * @brief オシレーターのベロシティボリュームを設定
 *
 * @param mem オシレーターメモリ
 * @param velocity ベロシティ値
 */
void Oscillator::setVelocity(Memory& mem, uint8_t velocity) {
    const float vel = AudioMath::velocityToAmplitude(velocity);
    mem.vel_vol = static_cast<int16_t>(vel * 1024.0f);
}

/**
 * @brief oscillatorの位相をリセット
 *
 * @param phase 位相
*/
void Oscillator::setPhase(Memory& mem, uint32_t phase) {
    mem.phase = phase;
}

/** @brief オシレーターを有効化 */
void Oscillator::enable() {
    enabled = true;
}

/** @brief オシレーターを無効化 */
void Oscillator::disable() {
    enabled = false;
}

/** @brief オシレーターの状態をリセット */
void Oscillator::reset(Memory& mem) {
    mem.phase = 0;
    mem.delta = 0;
    mem.vel_vol = 0;
}

/**
 * @brief オシレーターの音量を設定
 *
 * @param level 0~1024 (1.0)
 */
void Oscillator::setLevel(int16_t level) {
    this->level = clamp_level(level);
}

void Oscillator::setLevelNonLinear(uint8_t level) {
    uint8_t clamped_level = std::clamp<uint8_t>(level, 0, 99);
    this->level = level_table[clamped_level];
}

/**
 * @brief 波形テーブルを設定
 *
 * @param table_id id
 */
void Oscillator::setWavetable(uint8_t table_id) {
    if(table_id < 4) {
        wavetable = WAVETABLES[table_id].data;
        wavetable_size = WAVETABLES[table_id].size;
        bit_padding = AudioMath::bitPadding32(wavetable_size);
    }
}

void Oscillator::setCoarse(float coarse) {
    this->coarse = clamp_coarse(coarse);
}

void Oscillator::setFine(float fine_level) {
    this->fine_level = clamp_fine(fine_level);
}

void Oscillator::setDetune(int8_t detune_cents) {
    this->detune_cents = clamp_detune(detune_cents);
}