#pragma once

#include "handlers/audio.h"
#include "modules/envelope.h"
#include "utils/math.h"
#include "utils/wavetable.h"

class Oscillator {
public:
    struct Memory {
        uint32_t phase;
        uint32_t delta;
        float vel_vol;
    };

    Oscillator() {
        init();
    }

    void setFrequency(Memory& mem, uint8_t note);
    void setFrequency(Memory& mem, float freq);
    void setVelocity(Memory& mem, uint8_t velocity);
    void setPhase(Memory& mem, uint32_t phase);
    int16_t getSample(Memory& mem, uint8_t note_id);
    void update(Memory& mem, uint8_t note_id);
    void reset(Memory& mem);
    void enable();
    void disable();
    bool isActive();
    void setModulation(
        Oscillator* mod_osc,
        Envelope* mod_env,
        Oscillator::Memory* mod_osc_mems,
        Envelope::Memory* mod_env_mems
    );
    void setLoopback(bool loopback);
    void setLevel(float level);
    void setWavetable(uint8_t table_id);

private:
    // 定数
    static constexpr float F_1ULL32 = (float)(1ULL << 32);

    // OSC設定
    uint8_t bit_padding;
    int16_t* wavetable;
    size_t wavetable_size;
    bool enabled;
    float level;

    // モジュレーション関連
    bool loopback;
    Oscillator* mod_osc;
    Envelope* mod_env;
    Oscillator::Memory* mod_osc_mems;
    Envelope::Memory* mod_env_mems;

    void init();
};