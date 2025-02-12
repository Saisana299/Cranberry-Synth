#pragma once

#include "handlers/audio.hpp"
#include "utils/math.hpp"

class Envelope {
private:
    uint32_t attack_samples = (static_cast<uint32_t>(5 * SAMPLE_RATE)) >> 10;
    uint32_t decay_samples = (static_cast<uint32_t>(10 * SAMPLE_RATE)) >> 10;
    uint32_t release_samples = (static_cast<uint32_t>(10 * SAMPLE_RATE)) >> 10;
    int16_t sustain_level = 1 << 10;

    int32_t attack_inv = ((1U << 16) + (attack_samples >> 1)) / attack_samples;
    int32_t decay_inv = ((1U << 16) + (decay_samples >> 1)) / decay_samples;
    int32_t release_inv = ((1U << 16) + (release_samples >> 1)) / release_samples;

public:

    enum class State {
        Attack, Decay, Sustain, Release
    };

    struct Memory {
        State state = State::Attack;
        uint32_t elapsed = 0;
        int16_t current_level = 0;
        int16_t prev_level = 0;
    };

    /**
     * @brief 現在のレベルを返します
     *
     * @return int16_t Min: 0, Max: 1024
     */
    inline int16_t currentLevel(Memory& mem) {
        return mem.current_level;
    }

    /**
     * @brief エンベロープ終了判定
     *
     * @return 終了していれば `true` を返す
     */
    inline bool isFinished(Memory& mem) {
        return (mem.state == State::Release && mem.current_level == 0);
    }

    void reset(Memory& mem);
    void release(Memory& mem);
    void update(Memory& mem);

    void setAttack(int16_t attack_ms);
    void setDecay(int16_t decay_ms);
    void setRelease(int16_t release_ms);
    void setSustain(int16_t sustain_level);
};