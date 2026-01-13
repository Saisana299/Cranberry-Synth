#pragma once

#include <cstdint>
#include <array>

// プリセット最大数
constexpr uint8_t MAX_PRESETS = 4;

// オペレータープリセット構造体
struct OperatorPreset {
    // オシレーター設定
    uint8_t wavetable_id = 0;    // 波形テーブルID (0-3: sine, triangle, saw, square)
    uint8_t level = 0;           // レベル (0-99)
    float coarse = 1.0f;         // 粗調整 (0.0-31.0)
    float fine = 0.0f;           // 微調整 (0.0-99.0)
    int8_t detune = 0;           // デチューン (-7 to 7)

    // エンベロープ設定
    uint8_t attack  = 0;         // アタック (0-99)
    uint8_t decay   = 0;        // ディケイ (0-99)
    uint8_t sustain = 99;        // サステイン (0-99)
    uint8_t release = 0;        // リリース (0-99)

    // オペレーター有効/無効
    bool enabled = false;
};

// エフェクトプリセット構造体
struct EffectPreset {
    // ディレイ設定
    bool delay_enabled = false;
    int32_t delay_time = 256;          // ディレイタイム (1-300ms)
    int32_t delay_level = 307;         // ディレイレベル (0-1024)
    int32_t delay_feedback = 512;      // ディレイフィードバック (0-1024)

    // ローパスフィルタ設定
    bool lpf_enabled = false;
    float lpf_cutoff = 20000.0f;       // カットオフ周波数 (20-20000 Hz)
    float lpf_resonance = 0.70710678f; // Q値 (0.1-10.0)
    int16_t lpf_mix = 1024;            // ミックス量 (0-1024)

    // ハイパスフィルタ設定
    bool hpf_enabled = false;
    float hpf_cutoff = 20.0f;          // カットオフ周波数 (20-20000 Hz)
    float hpf_resonance = 0.70710678f; // Q値 (0.1-10.0)
    int16_t hpf_mix = 1024;            // ミックス量 (0-1024)
};

// シンセサイザープリセット構造体
struct SynthPreset {
    const char* name;                        // プリセット名
    uint8_t algorithm_id = 0;                // アルゴリズムID
    uint8_t feedback = 0;                    // フィードバック量 (0-7)
    std::array<OperatorPreset, 6> operators; // 6つのオペレーター設定
    EffectPreset effects;                    // エフェクト設定
};

// デフォルトプリセット管理クラス
class DefaultPresets {
public:
    /**
     * @brief プリセットIDからプリセットを取得
     *
     * @param id プリセットID (0-31)
     * @return const SynthPreset& プリセットデータへの参照
     */
    static const SynthPreset& get(uint8_t id) {
        if (id >= MAX_PRESETS) id = 0;
        return presets_[id];
    }

    /**
     * @brief プリセット総数を取得
     *
     * @return uint8_t プリセット数
     */
    static constexpr uint8_t count() {
        return MAX_PRESETS;
    }

private:
    // デフォルトプリセットデータ
    static inline const SynthPreset presets_[MAX_PRESETS] = {

        // --- Preset 0: Electric Piano ---
        {
            "E.Piano",      // name
            4,              // algorithm_id
            0,              // feedback
            {{              // operators
                // Operator 0 (Carrier 1)
                {
                    0,      // wavetable_id (sine)
                    99,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    3,      // detune
                    0,      // attack
                    60,     // decay
                    0,      // sustain
                    80,     // release
                    true    // enabled
                },
                // Operator 1 (Modulator 1)
                {
                    0,      // wavetable_id (sine)
                    35,     // level
                    14.0f,  // coarse
                    0.0f,   // fine
                    0,      // detune
                    0,      // attack
                    60,     // decay
                    0,      // sustain
                    0,      // release
                    true    // enabled
                },
                // Operator 2 (Carrier 2)
                {
                    0,      // wavetable_id (sine)
                    99,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune
                    0,      // attack
                    94,     // decay
                    0,      // sustain
                    80,     // release
                    true    // enabled
                },
                // Operator 3 (Modulator 2)
                {
                    0,      // wavetable_id (sine)
                    89,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune
                    0,      // attack
                    94,     // decay
                    0,      // sustain
                    80,     // release
                    true    // enabled
                },
                // Operator 4 (Carrier 3)
                {
                    0,      // wavetable_id (sine)
                    99,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    -7,     // detune
                    0,      // attack
                    94,     // decay
                    0,      // sustain
                    80,     // release
                    true    // enabled
                },
                // Operator 5 (Modulator 3)
                {
                    0,      // wavetable_id (sine)
                    79,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    +7,     // detune
                    0,      // attack
                    94,     // decay
                    0,      // sustain
                    80,     // release
                    true    // enabled
                }
            }},
            {   // effects
                false, 256, 307, 512,
                true, 6000.0f, 0.70710678f, 1024,
                false, 20.0f, 0.70710678f, 1024
            }
        },

        // --- Preset 1: Bass ---
        {
            "Bass",         // name
            0,              // algorithm_id
            0,              // feedback
            {{              // operators
                // Operator 0 (Carrier 1)
                {
                    0,      // wavetable_id (sine)
                    99,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune
                    0,      // attack
                    30,     // decay
                    70,     // sustain
                    40,     // release
                    true    // enabled
                },
                // Operator 1 (Modulator 1)
                {
                    0,      // wavetable_id (sine)
                    50,     // level
                    2.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune
                    0,      // attack
                    40,     // decay
                    60,     // sustain
                    30,     // release
                    true    // enabled
                },
                // Operators 2-5 disabled
                {0, 0, 1.0f, 0.0f, 0, 0, 50, 80, 50, false},
                {0, 0, 1.0f, 0.0f, 0, 0, 50, 80, 50, false},
                {0, 0, 1.0f, 0.0f, 0, 0, 50, 80, 50, false},
                {0, 0, 1.0f, 0.0f, 0, 0, 50, 80, 50, false}
            }},
            {   // effects
                false, 256, 307, 512,
                false, 20000.0f, 0.70710678f, 1024,
                false, 20.0f, 0.70710678f, 1024
            }
        },

        // --- Preset 2: Pad ---
        {
            "Pad",          // name
            0,              // algorithm_id
            0,              // feedback
            {{              // operators
                // Operator 0 (Carrier 1)
                {
                    0,      // wavetable_id (sine)
                    80,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune
                    50,     // attack
                    70,     // decay
                    90,     // sustain
                    80,     // release
                    true    // enabled
                },
                // Operator 1 (Modulator 1)
                {
                    0,      // wavetable_id (sine)
                    20,     // level
                    1.0f,   // coarse
                    0.0f,  // fine
                    0,      // detune
                    55,     // attack
                    75,     // decay
                    85,     // sustain
                    85,     // release
                    true    // enabled
                },
                // Operator 2 (Carrier 2)
                {
                    0,      // wavetable_id (sine)
                    75,     // level
                    2.0f,   // coarse
                    0.0f,   // fine
                    5,      // detune
                    52,     // attack
                    72,     // decay
                    88,     // sustain
                    82,     // release
                    true    // enabled
                },
                // Operators 3-5 disabled
                {0, 0, 1.0f, 0.0f, 0, 0, 50, 80, 50, false},
                {0, 0, 1.0f, 0.0f, 0, 0, 50, 80, 50, false},
                {0, 0, 1.0f, 0.0f, 0, 0, 50, 80, 50, false}
            }},
            {   // effects
                false, 256, 307, 512,
                false, 20000.0f, 0.70710678f, 1024,
                false, 20.0f, 0.70710678f, 1024
            }
        },

        // --- Preset 3: Lead ---
        {
            "Lead",         // name
            0,              // algorithm_id
            0,              // feedback
            {{              // operators
                // Operator 0 (Carrier 1)
                {
                    2,      // wavetable_id (saw)
                    99,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune
                    0,      // attack
                    50,     // decay
                    85,     // sustain
                    50,     // release
                    true    // enabled
                },
                // Operator 1 (Modulator 1)
                {
                    0,      // wavetable_id (sine)
                    40,     // level
                    3.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune
                    0,      // attack
                    45,     // decay
                    75,     // sustain
                    45,     // release
                    true    // enabled
                },
                // Operators 2-5 disabled
                {0, 0, 1.0f, 0.0f, 0, 0, 50, 80, 50, false},
                {0, 0, 1.0f, 0.0f, 0, 0, 50, 80, 50, false},
                {0, 0, 1.0f, 0.0f, 0, 0, 50, 80, 50, false},
                {0, 0, 1.0f, 0.0f, 0, 0, 50, 80, 50, false}
            }},
            {   // effects
                false, 256, 307, 512,
                false, 20000.0f, 0.70710678f, 1024,
                false, 20.0f, 0.70710678f, 1024
            }
        },
    };
};
