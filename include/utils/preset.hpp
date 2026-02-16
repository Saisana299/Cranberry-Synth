#pragma once

#include <cstdint>
#include <array>
#include "types.hpp"

// プリセット最大数
constexpr uint8_t MAX_PRESETS = 5;

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

    // Keyboard Level Scaling
    uint8_t kbd_break_point = 39;   // ブレークポイント (0-99, 39=C3)
    uint8_t kbd_left_depth = 0;     // 左側スケーリング深さ (0-99)
    uint8_t kbd_right_depth = 0;    // 右側スケーリング深さ (0-99)
    uint8_t kbd_left_curve = 0;     // 左側カーブ (0-3: -LN, -EX, +EX, +LN)
    uint8_t kbd_right_curve = 0;    // 右側カーブ (0-3: -LN, -EX, +EX, +LN)

    // ベロシティ感度
    uint8_t velocity_sens = 7;      // ベロシティ感度 (0-7, 0=感度なし, 7=最大感度)

    // オペレーター有効/無効
    bool enabled = false;

    // LFO AM感度 (AMS) — enabled の後に配置（既存プリセット初期化との互換性維持）
    uint8_t amp_mod_sens = 0;       // Amp Mod Sensitivity (0-3, 0=OFF, 3=最大)
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
    float hpf_cutoff = 120.0f;          // カットオフ周波数 (20-20000 Hz)
    float hpf_resonance = 0.70710678f; // Q値 (0.1-10.0)
    Gain_t hpf_mix = Q15_MAX;          // ミックス量 (0-Q15_MAX)

    // コーラス設定
    bool chorus_enabled = false;
    uint8_t chorus_rate = 20;           // LFO速度 (1-99 → 0.1-10Hz)
    uint8_t chorus_depth = 50;          // 変調深さ (0-99)
    Gain_t chorus_mix = 16384;          // ウェットミックス (Q15: 50% = 16384)
};

/**
 * @brief LFOプリセット構造体
 *
 * LFO波形:
 *   0=Triangle, 1=SawDown, 2=SawUp, 3=Square, 4=Sine, 5=Sample&Hold
 *
 * Pitch Mod Sensitivity (P MODE SENS): 0-7
 *   オペレーター全体にかかるピッチモジュレーション感度
 *   0=OFF, 7=最大
 */
struct LfoPreset {
    uint8_t wave = 0;             // LFO WAVE (0-5)
    uint8_t speed = 35;           // LFO SPEED (0-99)
    uint8_t delay = 0;            // LFO DELAY (0-99)
    uint8_t pm_depth = 0;         // LFO PM DEPTH (0-99)
    uint8_t am_depth = 0;         // LFO AM DEPTH (0-99)
    uint8_t pitch_mod_sens = 3;   // P MODE SENS (0-7)
    bool key_sync = true;         // LFO KEY SYNC (true=ON)
    bool osc_key_sync = true;     // OSC KEY SYNC (true=ON)
};

// シンセサイザープリセット構造体
struct SynthPreset {
    const char* name;                        // プリセット名
    uint8_t algorithm_id = 0;                // アルゴリズムID
    uint8_t feedback = 0;                    // フィードバック量 (0-7)
    std::array<OperatorPreset, 6> operators; // 6つのオペレーター設定
    EffectPreset effects;                    // エフェクト設定
    LfoPreset lfo;                           // LFO設定
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
                    99, 99, 99, 50, // rate1,2,3,4
                    99, 99, 99, 0, // level1,2,3,4
                    0,      // rate_scaling
                    39, 0, 0, 0, 0,  // kbd_break_point, kbd_left_depth, kbd_right_depth, kbd_left_curve, kbd_right_curve
                    7,      // velocity_sens
                    true    // enabled
                },
                {},  // Operator 1 (無効・デフォルト値)
                {},  // Operator 2 (無効・デフォルト値)
                {},  // Operator 3 (無効・デフォルト値)
                {},  // Operator 4 (無効・デフォルト値)
                {}   // Operator 5 (無効・デフォルト値)
            }},
            {}, // effects (default)
            {}  // lfo (default)
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
                    99, 32, 12, 42, // rate1,2,3,4
                    98, 75, 0, 0, // level1,2,3,4
                    0,      // rate_scaling
                    39, 99, 40, 3, 0,  // KLS (disabled)
                    7,      // velocity_sens
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
                    99, 48, 10, 13, // rate1,2,3,4
                    99, 81, 59, 0, // level1,2,3,4
                    0,      // rate_scaling
                    39, 0, 10, 1, 2,  // KLS (disabled)
                    7,      // velocity_sens
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
                    99, 40, 10, 40, // rate1,2,3,4
                    99, 27, 0, 0, // level1,2,3,4
                    0,      // rate_scaling
                    39, 0, 16, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
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
                    84, 24, 10, 29, // rate1,2,3,4
                    98, 98, 36, 0, // level1,2,3,4
                    0,      // rate_scaling
                    39, 0, 6, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
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
                    82, 26, 10, 27, // rate1,2,3,4
                    99, 91, 0, 0, // level1,2,3,4
                    0,      // rate_scaling
                    39, 2, 8, 3, 0,  // KLS (disabled)
                    7,      // velocity_sens
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
                    96, 76, 10, 31, // rate1,2,3,4
                    99, 92, 0, 0, // level1,2,3,4
                    0,      // rate_scaling
                    39, 7, 0, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                }
            }},
            {}, // effects (default)
            {}  // lfo (default)
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
                    99, 99, 23, 39, // rate1,2,3,4
                    99, 99, 0, 0, // level1,2,3,4
                    3,      // rate_scaling
                    38, 0, 0, 3, 0,  // KLS (disabled)
                    7,      // velocity_sens
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
                    95, 35, 23, 28, // rate1,2,3,4
                    99, 70, 0, 0, // level1,2,3,4
                    4,      // rate_scaling
                    40, 0, 15, 3, 0,  // KLS (disabled)
                    7,      // velocity_sens
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
                    95, 48, 28, 24, // rate1,2,3,4
                    94, 79, 0, 0, // level1,2,3,4
                    7,      // rate_scaling
                    52, 10, 0, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
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
                    59, 99, 23, 39, // rate1,2,3,4
                    66, 99, 0, 0, // level1,2,3,4
                    3,      // rate_scaling
                    39, 0, 0, 3, 0,  // KLS (disabled)
                    7,      // velocity_sens
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
                    95, 35, 23, 28, // rate1,2,3,4
                    99, 70, 0, 0, // level1,2,3,4
                    4,      // rate_scaling
                    40, 0, 15, 3, 0,  // KLS (disabled)
                    7,      // velocity_sens
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
                    95, 48, 28, 24, // rate1,2,3,4
                    93, 78, 0, 0, // level1,2,3,4
                    7,      // rate_scaling
                    48, 10, 11, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                }
            }},
            {}, // effects (default)
            {}  // lfo (default)
        },
        // --- Preset 4: TUB BELLS ---
        {
            "TUB BELLS",    // name
            4,              // algorithm_id
            7,              // feedback
            {{              // operators
                // Operator 0
                {
                    0,      // wavetable_id (sine)
                    95,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    2,      // detune
                    false, // is_fixed
                    95, 33, 71, 25, // rate1,2,3,4
                    99, 0, 32, 0, // level1,2,3,4
                    2,      // rate_scaling
                    4, 0, 0, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 1
                {
                    0,      // wavetable_id (sine)
                    78,     // level
                    2.0f,   // coarse
                    75.0f,   // fine
                    3,      // detune
                    false, // is_fixed
                    98, 12, 71, 28, // rate1,2,3,4
                    99, 0, 32, 0, // level1,2,3,4
                    2,      // rate_scaling
                    4, 0, 0, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 2
                {
                    0,      // wavetable_id (sine)
                    99,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    -5,     // detune
                    false, // is_fixed
                    95, 33, 71, 25, // rate1,2,3,4
                    94, 0, 32, 0, // level1,2,3,4
                    2,      // rate_scaling
                    4, 0, 0, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 3
                {
                    0,      // wavetable_id (sine)
                    75,     // level
                    2.0f,   // coarse
                    75.0f,   // fine
                    -2,     // detune
                    false, // is_fixed
                    98, 12, 71, 28, // rate1,2,3,4
                    99, 0, 32, 0, // level1,2,3,4
                    2,      // rate_scaling
                    4, 0, 0, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 4
                {
                    0,      // wavetable_id (sine)
                    99,     // level
                    2.0f,  // coarse
                    51.0f,   // fine
                    0,     // detune
                    true, // is_fixed
                    76, 78, 71, 70, // rate1,2,3,4
                    99, 0, 0, 0, // level1,2,3,4
                    2,      // rate_scaling
                    4, 0, 0, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 5
                {
                    0,      // wavetable_id (sine)
                    85,     // level
                    2.0f,   // coarse
                    0.0f,  // fine
                    -7,      // detune
                    false, // is_fixed
                    98, 91, 0, 28, // rate1,2,3,4
                    99, 0, 0, 0, // level1,2,3,4
                    2,      // rate_scaling
                    4, 0, 0, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                }
            }},
            {}, // effects (default)
            {}  // lfo (default)
        },
        // --- Preset 5: E.PIANO 1 ---
        {
            "E.PIANO 1",    // name
            4,              // algorithm_id
            6,              // feedback
            {{              // operators
                // Operator 0
                {
                    0,      // wavetable_id (sine)
                    99,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    3,      // detune
                    false, // is_fixed
                    96, 25, 25, 67, // rate1,2,3,4
                    99, 75, 0, 0, // level1,2,3,4
                    3,      // rate_scaling
                    0, 0, 0, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 1
                {
                    0,      // wavetable_id (sine)
                    58,     // level
                    14.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune
                    false, // is_fixed
                    95, 50, 35, 78, // rate1,2,3,4
                    99, 75, 0, 0, // level1,2,3,4
                    3,      // rate_scaling
                    0, 0, 0, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 2
                {
                    0,      // wavetable_id (sine)
                    99,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    0,     // detune
                    false, // is_fixed
                    95, 20, 20, 50, // rate1,2,3,4
                    99, 95, 0, 0, // level1,2,3,4
                    3,      // rate_scaling
                    0, 0, 0, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 3
                {
                    0,      // wavetable_id (sine)
                    89,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    0,     // detune
                    false, // is_fixed
                    96, 29, 20, 50, // rate1,2,3,4
                    99, 95, 0, 0, // level1,2,3,4
                    3,      // rate_scaling
                    0, 0, 0, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 4
                {
                    0,      // wavetable_id (sine)
                    99,     // level
                    1.0f,  // coarse
                    0.0f,   // fine
                    -7,     // detune
                    false, // is_fixed
                    95, 20, 20, 50, // rate1,2,3,4
                    99, 95, 0, 0, // level1,2,3,4
                    3,      // rate_scaling
                    0, 0, 0, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 5
                {
                    0,      // wavetable_id (sine)
                    79,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    7,      // detune
                    false, // is_fixed
                    95, 29, 20, 50, // rate1,2,3,4
                    99, 95, 0, 0, // level1,2,3,4
                    3,      // rate_scaling
                    33, 0, 19, 0, 0,  // KLS (disabled)
                    7,      // velocity_sens
                    true    // enabled
                }
            }},
            {}, // effects (default)
            {}  // lfo (default)
        },
    };
};
