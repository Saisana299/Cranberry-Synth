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
    void reset(Memory& mem);
    void enable();
    void disable();

    void setLevel(int16_t level);
    void setLevelNonLinear(uint8_t level);
    void setWavetable(uint8_t table_id);
    void setCoarse(float coarse);
    void setFine(float fine_level);
    void setDetune(int8_t detune_cents);

    // 非線形レベルテーブル
    static int16_t level_table[100];
    static bool table_initialized;
    static void initTable() {
        if (table_initialized) return;

        for (size_t i = 0; i < 100; ++i) {
            float x = i / 99.0f;
            // log(0.5)/log(0.75) の計算結果リテラル
            const float exponent = 2.40942f;
            level_table[i] = static_cast<int16_t>(powf(x, exponent) * 1024.0f);
        }
        table_initialized = true;
    }

    /** @brief オシレーターの状態 */
    inline bool isActive() const {
        return enabled;
    }

    /**
     * @brief オシレーターの状態を更新
     *
     * @param mem オシレーターメモリ
     * @param note_id ノートID
    */
    FASTRUN inline void update(Memory& mem) {
        if(enabled) mem.phase += mem.delta;
    }

    /**
     * @brief oscillatorのサンプルを取得
     *
     * @param mem オシレーターメモリ
     * @param note_id ノートID
     * @return int16_t オシレーター出力サンプル
     */
    FASTRUN inline int16_t getSample(Memory& mem, int32_t mod_product = 0) {
        if(!enabled) return 0;

        // ローカル変数にキャッシュ
        const int16_t local_vel_vol = mem.vel_vol;
        const uint32_t local_phase = mem.phase;

        // キャリアのベース位相
        uint32_t base_phase = local_phase;

        // モジュレーションの位相オフセット
        const int32_t mod_phase_offset = mod_product * static_cast<int32_t>(MOD_PHASE_SCALE_FACTOR);

        // 位相計算
        uint32_t effective_phase = base_phase + static_cast<uint32_t>(mod_phase_offset);

        // 波形テーブルを参照する
        const size_t index = (effective_phase >> bit_padding) & (wavetable_size - 1); // サンプル数が2^Nである前提
        const int16_t sample = wavetable[index];

        // ベロシティレベルとオシレーターレベルを適用
        const int32_t scaling = (static_cast<int32_t>(local_vel_vol) * static_cast<int32_t>(level)) >> 10;
        return static_cast<int16_t>((static_cast<int32_t>(sample) * scaling) >> 10);
    }

private:
    // 定数
    static constexpr float PHASE_SCALE_FACTOR = static_cast<float>(1ULL << 32) / SAMPLE_RATE;
    static constexpr uint32_t MOD_PHASE_SCALE_FACTOR = 131072; // 2^17

    struct WavetableInfo {
        const int16_t* data;
        size_t size;
    };

    static constexpr WavetableInfo WAVETABLES[4] = {
        {Wavetable::sine,     sizeof(Wavetable::sine) / sizeof(Wavetable::sine[0])},
        {Wavetable::triangle, sizeof(Wavetable::triangle) / sizeof(Wavetable::triangle[0])},
        {Wavetable::saw,      sizeof(Wavetable::saw) / sizeof(Wavetable::saw[0])},
        {Wavetable::square,   sizeof(Wavetable::square) / sizeof(Wavetable::square[0])},
    };

    // パラメータ検証
    static inline int16_t clamp_level(int16_t value) {
        return std::clamp<int16_t>(value, 0, 1024);
    }

    static inline float clamp_coarse(float value) {
        return std::clamp<float>(value, 0.5f, 31.0f);
    }

    static inline float clamp_fine(float value) {
        return std::clamp<float>(value, 1.0f, 1.99f);
    }

    static inline int8_t clamp_detune(int8_t value) {
        return std::clamp<int8_t>(value, -7, 7);
    }

    // OSC設定
    uint8_t bit_padding; // コンストラクタで初期化
    const int16_t* wavetable = Wavetable::sine;
    size_t wavetable_size = sizeof(Wavetable::sine) / sizeof(Wavetable::sine[0]);
    bool enabled = false;
    int16_t level = 0;

    // ピッチ
    float coarse = 1.0f;
    float fine_level = 0.0f;
    int8_t detune_cents = 0;

    // ratioかfixedか
    bool is_fixed = false;
};