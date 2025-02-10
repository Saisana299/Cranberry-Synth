#pragma once

#include "handlers/audio.hpp"
#include "utils/math.hpp"

class Envelope {
private:
    uint32_t attack_samples = static_cast<uint32_t>(0.005f * SAMPLE_RATE);
    uint32_t decay_samples = static_cast<uint32_t>(0.01f * SAMPLE_RATE);
    uint32_t release_samples = static_cast<uint32_t>(0.01f * SAMPLE_RATE);
    float sustain_level = 1.0f;

    float attack_inv = 1.0f / static_cast<float>(attack_samples);
    float decay_inv = 1.0f / static_cast<float>(decay_samples);
    float release_inv = 1.0f / static_cast<float>(release_samples);

public:

    enum class State {
        Attack, Decay, Sustain, Release
    };

    struct Memory {
        State state = State::Attack;
        float elapsed = 0;
        float current_level = 0.0f;
        float prev_level = 0.0f;
    };

    /**
     * @brief 現在のレベルを返します
     *
     * @return float Min: 0.0, Max: 1.0
     */
    inline float currentLevel(Memory& mem) {
        return mem.current_level;
    }

    /**
     * @brief エンベロープ終了判定
     *
     * @return 終了していれば `true` を返す
     */
    inline bool isFinished(Memory& mem) {
        return (mem.state == State::Release && mem.current_level == 0.0f);
    }

    void reset(Memory& mem);
    void release(Memory& mem);
    void update(Memory& mem);

    void setAttack(float attack_ms);
    void setDecay(float decay_ms);
    void setRelease(float release_ms);
    void setSustain(float sustain_level);
};