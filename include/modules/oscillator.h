#pragma once

#include "handlers/audio.h"
#include "modules/envelope.h"
#include "utils/math.h"
#include "utils/wavetable.h"

class Oscillator {
public:
    struct Memory {
        uint32_t phase = 0;
        uint32_t delta = 0;
        float vel_vol = 0.0f;
    };

    Oscillator() {
        bit_padding = AudioMath::bitPadding32(wavetable_size);
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
    void setFeedback(bool is_feedback);
    void setLevel(float level);
    void setWavetable(uint8_t table_id);

private:
    // 定数
    static constexpr float F_1ULL32 = static_cast<float>(1ULL << 32);

    // OSC設定
    uint8_t bit_padding; // コンストラクタで初期化
    int16_t* wavetable = Wavetable::sine;
    size_t wavetable_size = sizeof(Wavetable::sine) / sizeof(Wavetable::sine[0]);
    bool enabled = false;
    float level = 0.0f;

    // ピッチ
    uint8_t coarse = 0;
    uint8_t fine = 0;
    int8_t tune = 0;

    // ratioかfixedか
    bool is_fixed = false;

    // モジュレーション関連
    bool is_feedback = false;
    float feedback = 0.0f;
    Oscillator* mod_osc = nullptr;
    Envelope* mod_env = nullptr;
    Oscillator::Memory* mod_osc_mems = nullptr;
    Envelope::Memory* mod_env_mems = nullptr;
};