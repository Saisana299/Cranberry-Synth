#pragma once

#include <cstdint>
#include <array>
#include "types.hpp"

// プリセット最大数
constexpr uint8_t MAX_PRESETS = 3;

/**
 * @brief オペレータープリセット構造体
 *
 * エンベロープは4つのRate(R1-R4)と4つのLevel(L1-L4)で構成:
 * - Phase1: 現在レベル → L1 (R1で移動) [アタック]
 * - Phase2: L1 → L2 (R2で移動) [ディケイ1]
 * - Phase3: L2 → L3 (R3で移動) [ディケイ2/サステイン]
 * - Phase4: → L4 (R4で移動) [リリース] ※ノートオフ時
 *
 * Rate: 0=最も遅い, 99=最も速い
 * Level: 0=無音, 99=最大音量
 */
struct OperatorPreset {
    // オシレーター設定
    uint8_t wavetable_id = 0;    // 波形テーブルID (0-3: sine, triangle, saw, square)
    uint8_t level = 0;           // レベル (0-99)
    float coarse = 1.0f;         // 粗調整 (0.0-31.0)
    float fine = 0.0f;           // 微調整 (0.0-99.0)
    int8_t detune = 0;           // デチューン (-7 to 7)
    bool is_fixed = false;       // FIXEDモード (true: 固定周波数, false: RATIO)

    // エンベロープ設定 (Rate/Level)
    uint8_t rate1 = 99;          // Rate1: アタックレート (0-99)
    uint8_t rate2 = 99;          // Rate2: ディケイ1レート (0-99)
    uint8_t rate3 = 99;          // Rate3: ディケイ2レート (0-99)
    uint8_t rate4 = 99;          // Rate4: リリースレート (0-99)
    uint8_t level1 = 99;         // Level1: アタック到達レベル (0-99)
    uint8_t level2 = 99;         // Level2: ディケイ1到達レベル (0-99)
    uint8_t level3 = 99;         // Level3: サステインレベル (0-99)
    uint8_t level4 = 0;          // Level4: リリース到達レベル (0-99)

    // Rate Scaling
    uint8_t rate_scaling = 0;    // Rate Scaling sensitivity (0-7): 高音ほどエンベロープが速くなる

    // オペレーター有効/無効
    bool enabled = false;
};

// エフェクトプリセット構造体
struct EffectPreset {
    // ディレイ設定
    bool delay_enabled = false;
    int32_t delay_time = 80;           // ディレイタイム (1-300ms)
    Gain_t delay_level = 9830;         // ディレイレベル (Q15: 30% = 9830)
    Gain_t delay_feedback = 16384;     // ディレイフィードバック (Q15: 50% = 16384)

    // ローパスフィルタ設定
    bool lpf_enabled = false;
    float lpf_cutoff = 20000.0f;       // カットオフ周波数 (20-20000 Hz)
    float lpf_resonance = 0.70710678f; // Q値 (0.1-10.0)
    Gain_t lpf_mix = Q15_MAX;          // ミックス量 (0-Q15_MAX)

    // ハイパスフィルタ設定
    bool hpf_enabled = false;
    float hpf_cutoff = 20.0f;          // カットオフ周波数 (20-20000 Hz)
    float hpf_resonance = 0.70710678f; // Q値 (0.1-10.0)
    Gain_t hpf_mix = Q15_MAX;          // ミックス量 (0-Q15_MAX)
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
        // --- Preset 1: Simple Sine ---
        {
            "Simple Sine",  // name
            0,              // algorithm_id
            0,              // feedback
            {{              // operators
                // Operator 0
                {
                    0,      // wavetable_id
                    99,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune
                    false,  // is_fixed
                    99,     // rate1
                    99,     // rate2
                    99,     // rate3
                    50,     // rate4
                    99,     // level1
                    99,     // level2
                    99,     // level3
                    0,      // level4
                    0,      // rate_scaling
                    true    // enabled
                },
                // Operator 1 (無効)
                {
                    0, 0, 1.0f, 0.0f, 0, false,
                    99, 99, 99, 99, 99, 99, 99, 0,
                    0, false
                },
                // Operator 2 (無効)
                {
                    0, 0, 1.0f, 0.0f, 0, false,
                    99, 99, 99, 99, 99, 99, 99, 0,
                    0, false
                },
                // Operator 3 (無効)
                {
                    0, 0, 1.0f, 0.0f, 0, false,
                    99, 99, 99, 99, 99, 99, 99, 0,
                    0, false
                },
                // Operator 4 (無効)
                {
                    0, 0, 1.0f, 0.0f, 0, false,
                    99, 99, 99, 99, 99, 99, 99, 0,
                    0, false
                },
                // Operator 5 (無効)
                {
                    0, 0, 1.0f, 0.0f, 0, false,
                    99, 99, 99, 99, 99, 99, 99, 0,
                    0, false
                }
            }},
            {   // effects
                false, 256, 307, 512,
                false, 20000.0f, 0.70710678f, 1024,
                false, 20.0f, 0.70710678f, 1024
            }
        },
        // --- Preset 2: Melodrama ---
        {
            "Melodrama",    // name
            1,              // algorithm_id
            7,              // feedback
            {{              // operators
                // Operator 0
                {
                    0,      // wavetable_id (sine)
                    99,     // level
                    4.0f,   // coarse
                    0.0f,   // fine
                    7,      // detune
                    false, // is_fixed
                    99,     // rate1
                    32,     // rate2
                    12,     // rate3
                    42,     // rate4
                    98,     // level1
                    75,     // level2
                    0,      // level3
                    0,      // level4
                    0,      // rate_scaling
                    true    // enabled
                },
                // Operator 1
                {
                    0,      // wavetable_id (sine)
                    81,     // level
                    4.0f,   // coarse
                    0.0f,   // fine
                    7,      // detune
                    false, // is_fixed
                    99,     // rate1
                    48,     // rate2
                    10,     // rate3
                    13,     // rate4
                    99,     // level1
                    81,     // level2
                    59,     // level3
                    0,      // level4
                    0,      // rate_scaling
                    true    // enabled
                },
                // Operator 2
                {
                    0,      // wavetable_id (sine)
                    99,     // level
                    2.0f,   // coarse
                    0.0f,   // fine
                    -7,     // detune
                    false, // is_fixed
                    99,     // rate1
                    40,     // rate2
                    10,     // rate3
                    40,     // rate4
                    99,     // level1
                    27,     // level2
                    0,      // level3
                    0,      // level4
                    0,      // rate_scaling
                    true    // enabled
                },
                // Operator 3
                {
                    0,      // wavetable_id (sine)
                    76,     // level
                    6.0f,   // coarse
                    0.0f,   // fine
                    -7,     // detune
                    false, // is_fixed
                    84,     // rate1
                    24,     // rate2
                    10,     // rate3
                    29,     // rate4
                    98,     // level1
                    98,     // level2
                    36,     // level3
                    0,      // level4
                    0,      // rate_scaling
                    true    // enabled
                },
                // Operator 4
                {
                    0,      // wavetable_id (sine)
                    86,     // level
                    12.0f,  // coarse
                    0.0f,   // fine
                    -7,     // detune
                    false, // is_fixed
                    82,     // rate1
                    26,     // rate2
                    10,     // rate3
                    27,     // rate4
                    99,     // level1
                    91,     // level2
                    0,      // level3
                    0,      // level4
                    0,      // rate_scaling
                    true    // enabled
                },
                // Operator 5
                {
                    0,      // wavetable_id (sine)
                    80,     // level
                    0.0f,   // coarse
                    65.0f,  // fine
                    0,      // detune
                    true, // is_fixed
                    96,     // rate1
                    76,     // rate2
                    10,     // rate3
                    31,     // rate4
                    99,     // level1
                    92,     // level2
                    0,      // level3
                    0,      // level4
                    0,      // rate_scaling
                    true    // enabled
                }
            }},
            {   // effects
                false, 256, 307, 512,
                false, 6000.0f, 0.70710678f, 1024,
                false, 20.0f, 0.70710678f, 1024
            }
        },
        // --- Preset 3: KinzkHarp ---
        {
            "KinzkHarp",    // name
            2,              // algorithm_id
            7,              // feedback
            {{              // operators
                // Operator 0
                {
                    0,      // wavetable_id (sine)
                    94,     // level
                    2.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune
                    false, // is_fixed
                    99,     // rate1
                    99,     // rate2
                    23,     // rate3
                    39,     // rate4
                    99,     // level1
                    99,     // level2
                    0,      // level3
                    0,      // level4
                    0,      // rate_scaling
                    true    // enabled
                },
                // Operator 1
                {
                    0,      // wavetable_id (sine)
                    73,     // level
                    6.0f,   // coarse
                    0.0f,   // fine
                    -2,      // detune
                    false, // is_fixed
                    95,     // rate1
                    35,     // rate2
                    23,     // rate3
                    28,     // rate4
                    99,     // level1
                    70,     // level2
                    0,     // level3
                    0,      // level4
                    0,      // rate_scaling
                    true    // enabled
                },
                // Operator 2
                {
                    0,      // wavetable_id (sine)
                    91,     // level
                    16.0f,   // coarse
                    0.0f,   // fine
                    -6,     // detune
                    false, // is_fixed
                    95,     // rate1
                    48,     // rate2
                    28,     // rate3
                    24,     // rate4
                    94,     // level1
                    79,     // level2
                    0,      // level3
                    0,      // level4
                    0,      // rate_scaling
                    true    // enabled
                },
                // Operator 3
                {
                    0,      // wavetable_id (sine)
                    99,     // level
                    2.0f,   // coarse
                    0.0f,   // fine
                    -7,     // detune
                    false, // is_fixed
                    59,     // rate1
                    99,     // rate2
                    23,     // rate3
                    39,     // rate4
                    66,     // level1
                    99,     // level2
                    0,     // level3
                    0,      // level4
                    0,      // rate_scaling
                    true    // enabled
                },
                // Operator 4
                {
                    0,      // wavetable_id (sine)
                    75,     // level
                    6.0f,  // coarse
                    0.0f,   // fine
                    4,     // detune
                    false, // is_fixed
                    95,     // rate1
                    35,     // rate2
                    23,     // rate3
                    28,     // rate4
                    99,     // level1
                    70,     // level2
                    0,      // level3
                    0,      // level4
                    0,      // rate_scaling
                    true    // enabled
                },
                // Operator 5
                {
                    0,      // wavetable_id (sine)
                    99,     // level
                    14.0f,   // coarse
                    0.0f,  // fine
                    1,      // detune
                    false, // is_fixed
                    95,     // rate1
                    48,     // rate2
                    28,     // rate3
                    24,     // rate4
                    93,     // level1
                    78,     // level2
                    0,      // level3
                    0,      // level4
                    0,      // rate_scaling
                    true    // enabled
                }
            }},
            {   // effects
                false, 256, 307, 512,
                false, 6000.0f, 0.70710678f, 1024,
                false, 20.0f, 0.70710678f, 1024
            }
        },
    };
};
