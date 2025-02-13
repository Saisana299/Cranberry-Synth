#pragma once

#include "handlers/audio.hpp"
#include "modules/envelope.hpp"
#include "utils/math.hpp"
#include "utils/wavetable.hpp"

class Oscillator {
public:
    struct Memory {
        uint32_t phase = 0;
        uint32_t delta = 0;
        int16_t vel_vol = 0;
    };

    Oscillator() {
        bit_padding = AudioMath::bitPadding32(wavetable_size);
    }

    void setFrequency(Memory& mem, uint8_t note);
    void setVelocity(Memory& mem, uint8_t velocity);
    void setPhase(Memory& mem, uint32_t phase);
    int16_t getSample(Memory& mem, uint8_t note_id);
    void update(Memory& mem, uint8_t note_id);
    void reset(Memory& mem);
    void enable();
    void disable();
    void setModulation(
        Oscillator* mod_osc,
        Envelope* mod_env,
        Oscillator::Memory* mod_osc_mems,
        Envelope::Memory* mod_env_mems
    );
    void setFeedback(bool is_feedback);
    void setLevel(int16_t level);
    void setLevelNonLinear(uint8_t level);
    void setWavetable(uint8_t table_id);
    void setCoarse(float coarse);
    void setFine(float fine_level);
    void setDetune(int8_t detune_cents);

    /** @brief オシレーターの状態 */
    inline bool isActive() {
        return enabled;
    }

private:
    // 定数
    static constexpr float PHASE_SCALE_FACTOR = static_cast<float>(1ULL << 32) / SAMPLE_RATE;

    // OSC設定
    uint8_t bit_padding; // コンストラクタで初期化
    int16_t* wavetable = Wavetable::sine;
    size_t wavetable_size = sizeof(Wavetable::sine) / sizeof(Wavetable::sine[0]);
    bool enabled = false;
    int16_t level = 0;

    // ピッチ
    float coarse = 1.0f;
    float fine_level = 0.0f;
    int8_t detune_cents = 0;

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