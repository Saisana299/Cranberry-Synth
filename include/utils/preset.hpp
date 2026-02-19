#pragma once

#include <cstdint>
#include <cmath>
#include <array>
#include "types.hpp"

// プリセット最大数
constexpr uint8_t MAX_PRESETS = 10;

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
// すべてのパラメータは直感的な 0-99 値で管理し、
// loadPreset() で内部表現（Hz, Q15等）に変換する
struct EffectPreset {
    // ディレイ設定
    bool delay_enabled = false;
    int32_t delay_time = 80;            // ディレイタイム (1-300ms)
    uint8_t delay_level = 30;           // ディレイレベル (0-99%)
    uint8_t delay_feedback = 50;        // ディレイフィードバック (0-99%)

    // ローパスフィルタ設定
    bool lpf_enabled = false;
    uint8_t lpf_cutoff = 99;            // カットオフ (0-99, 対数: 20Hz→20kHz)
    uint8_t lpf_resonance = 6;          // Q値 (0-99 → 0.1-10.0, 6≈0.707)
    uint8_t lpf_mix = 99;               // ミックス (0-99%)

    // ハイパスフィルタ設定
    bool hpf_enabled = false;
    uint8_t hpf_cutoff = 26;            // カットオフ (0-99, 対数: 20Hz→20kHz, 26≈120Hz)
    uint8_t hpf_resonance = 6;          // Q値 (0-99 → 0.1-10.0, 6≈0.707)
    uint8_t hpf_mix = 99;               // ミックス (0-99%)

    // コーラス設定
    bool chorus_enabled = false;
    uint8_t chorus_rate = 20;           // LFO速度 (1-99 → 0.1-10Hz)
    uint8_t chorus_depth = 50;          // 変調深さ (0-99)
    uint8_t chorus_mix = 50;            // ウェットミックス (0-99%)

    // リバーブ設定
    bool reverb_enabled = false;
    uint8_t reverb_room_size = 50;      // ルームサイズ (0-99)
    uint8_t reverb_damping = 50;        // ダンピング (0-99)
    uint8_t reverb_mix = 25;            // ウェットミックス (0-99%)

    // === 変換ユーティリティ ===

    /** @brief カットオフ 0-99 → 20-20000Hz (対数スケール) */
    static float cutoffToHz(uint8_t value) {
        return 20.0f * powf(1000.0f, static_cast<float>(value) / 99.0f);
    }

    /** @brief レゾナンス 0-99 → 0.1-10.0 (線形) */
    static float resonanceToQ(uint8_t value) {
        return 0.1f + static_cast<float>(value) / 99.0f * 9.9f;
    }

    /** @brief パーセント 0-99 → Q15 (0-32767) */
    static Gain_t toQ15(uint8_t value) {
        return static_cast<Gain_t>(static_cast<int32_t>(value) * Q15_MAX / 99);
    }

    /** @brief Q15 → 0-99 パーセント (逆変換) */
    static uint8_t fromQ15(Gain_t value) {
        return static_cast<uint8_t>(static_cast<int32_t>(value) * 99 / Q15_MAX);
    }

    /** @brief Hz → 0-99 カットオフ (逆変換) */
    static uint8_t hzToCutoff(float hz) {
        if (hz <= 20.0f) return 0;
        if (hz >= 20000.0f) return 99;
        return static_cast<uint8_t>(log10f(hz / 20.0f) / 3.0f * 99.0f + 0.5f);
    }

    /** @brief Q値 → 0-99 レゾナンス (逆変換) */
    static uint8_t qToResonance(float q) {
        return static_cast<uint8_t>((q - 0.1f) / 9.9f * 99.0f + 0.5f);
    }
};

/**
 * @brief マスタープリセット構造体
 *
 * シンセサイザーのグローバル設定:
 * - Level: マスターボリューム (Q15形式, 0-32767)
 * - Transpose: トランスポーズ (-24 ～ +24 半音)
 */
struct MasterPreset {
    uint8_t level = 70;            // マスターレベル (0-99%, デフォルト: 70 ≈ -3dB)
    int8_t transpose = 0;          // トランスポーズ (-24 ～ +24)
    uint8_t feedback = 0;          // フィードバック量 (0-7)
    uint8_t velocity_curve = 0;    // ベロシティカーブ (0=Linear, 1=Exp, 2=Log, 3=Fixed)
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
    std::array<OperatorPreset, 6> operators; // 6つのオペレーター設定
    EffectPreset effects;                    // エフェクト設定
    LfoPreset lfo;                           // LFO設定
    MasterPreset master;                     // マスター設定
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
            {}, // lfo (default)
            {}  // master (default)
        },
        // --- Preset 2: Triangle ---
        {
            "Triangle",     // name
            0,              // algorithm_id
            {{              // operators
                // Operator 0
                {
                    1,      // wavetable_id (triangle)
                    99,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune
                    false,  // is_fixed
                    99, 99, 99, 50, // rate1,2,3,4
                    99, 99, 99, 0, // level1,2,3,4
                    0,      // rate_scaling
                    39, 0, 0, 0, 0,  // KLS
                    7,      // velocity_sens
                    true    // enabled
                },
                {},  // Operator 1
                {},  // Operator 2
                {},  // Operator 3
                {},  // Operator 4
                {}   // Operator 5
            }},
            {}, // effects (default)
            {}, // lfo (default)
            {}  // master (default)
        },
        // --- Preset 3: Square ---
        {
            "Square",       // name
            0,              // algorithm_id
            {{              // operators
                // Operator 0
                {
                    3,      // wavetable_id (square)
                    99,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune
                    false,  // is_fixed
                    99, 99, 99, 50, // rate1,2,3,4
                    99, 99, 99, 0, // level1,2,3,4
                    0,      // rate_scaling
                    39, 0, 0, 0, 0,  // KLS
                    7,      // velocity_sens
                    true    // enabled
                },
                {},  // Operator 1
                {},  // Operator 2
                {},  // Operator 3
                {},  // Operator 4
                {}   // Operator 5
            }},
            {}, // effects (default)
            {}, // lfo (default)
            {}  // master (default)
        },
        // --- Preset 4: Sawtooth ---
        {
            "Sawtooth",     // name
            0,              // algorithm_id
            {{              // operators
                // Operator 0
                {
                    2,      // wavetable_id (saw)
                    99,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune
                    false,  // is_fixed
                    99, 99, 99, 50, // rate1,2,3,4
                    99, 99, 99, 0, // level1,2,3,4
                    0,      // rate_scaling
                    39, 0, 0, 0, 0,  // KLS
                    7,      // velocity_sens
                    true    // enabled
                },
                {},  // Operator 1
                {},  // Operator 2
                {},  // Operator 3
                {},  // Operator 4
                {}   // Operator 5
            }},
            {}, // effects (default)
            {}, // lfo (default)
            {}  // master (default)
        },
        // --- Preset 5: SUPERSAW ---
        {
            "SUPERSAW",     // name
            31,             // algorithm_id (全OP独立出力)
            {{              // operators
                // Operator 0: 中心
                {
                    2,      // wavetable_id (saw)
                    99,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    0,      // detune (center)
                    false,  // is_fixed
                    99, 40, 20, 50, // rate1,2,3,4
                    99, 90, 80, 0,  // level1,2,3,4
                    2,      // rate_scaling
                    39, 0, 0, 0, 0,  // KLS
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 1: +detune
                {
                    2,      // wavetable_id (saw)
                    92,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    15,     // detune (+15 cents)
                    false,  // is_fixed
                    99, 40, 20, 50, // rate1,2,3,4
                    99, 90, 80, 0,  // level1,2,3,4
                    2,      // rate_scaling
                    39, 0, 0, 0, 0,  // KLS
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 2: -detune
                {
                    2,      // wavetable_id (saw)
                    92,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    -15,    // detune (-15 cents)
                    false,  // is_fixed
                    99, 40, 20, 50, // rate1,2,3,4
                    99, 90, 80, 0,  // level1,2,3,4
                    2,      // rate_scaling
                    39, 0, 0, 0, 0,  // KLS
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 3: +大detune
                {
                    2,      // wavetable_id (saw)
                    82,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    35,     // detune (+35 cents)
                    false,  // is_fixed
                    99, 40, 20, 50, // rate1,2,3,4
                    99, 90, 80, 0,  // level1,2,3,4
                    2,      // rate_scaling
                    39, 0, 0, 0, 0,  // KLS
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 4: -大detune
                {
                    2,      // wavetable_id (saw)
                    82,     // level
                    1.0f,   // coarse
                    0.0f,   // fine
                    -35,    // detune (-35 cents)
                    false,  // is_fixed
                    99, 40, 20, 50, // rate1,2,3,4
                    99, 90, 80, 0,  // level1,2,3,4
                    2,      // rate_scaling
                    39, 0, 0, 0, 0,  // KLS
                    7,      // velocity_sens
                    true    // enabled
                },
                // Operator 5: +1オクターブ
                {
                    2,      // wavetable_id (saw)
                    70,     // level
                    2.0f,   // coarse (+1oct)
                    0.0f,   // fine
                    10,     // detune (+10 cents)
                    false,  // is_fixed
                    99, 40, 20, 50, // rate1,2,3,4
                    99, 85, 70, 0,  // level1,2,3,4
                    2,      // rate_scaling
                    39, 0, 0, 0, 0,  // KLS
                    7,      // velocity_sens
                    true    // enabled
                }
            }},
            {
                // ディレイ設定 (default)
                false, 80, 30, 50,
                // ローパスフィルタ (高域を少しカット)
                true, 90, 6, 99,
                // ハイパスフィルタ (default)
                false, 26, 6, 99,
                // コーラス設定
                true, 15, 40, 50
            }, // effects
            {}, // lfo (default)
            { 55, 0, 0 }  // master (level=55: 6オペレータ分の音量調整)
        },
        // --- Preset 6: Melodrama ---
        {
            "Melodrama",    // name
            1,              // algorithm_id
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
            {}, // lfo (default)
            { 70, 0, 7 }  // master (level=70, feedback=7)
        },
        // --- Preset 7: KinzkHarp ---
        {
            "KinzkHarp",    // name
            2,              // algorithm_id
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
            {}, // lfo (default)
            { 70, 0, 7 }  // master (level=70, feedback=7)
        },
        // --- Preset 8: TUB BELLS ---
        {
            "TUB BELLS",    // name
            4,              // algorithm_id
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
            {}, // lfo (default)
            { 70, 0, 7 }  // master (level=70, feedback=7)
        },
        // --- Preset 9: E.PIANO 1 ---
        {
            "E.PIANO 1",    // name
            4,              // algorithm_id
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
            {}, // lfo (default)
            { 70, 0, 6 }  // master (level=70, feedback=6)
        },
        // --- Preset 10: KinzkHarp EF ---
        {
            "KinzkHarp EF",    // name
            2,              // algorithm_id
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
            {
                // ディレイ設定
                true, 45, 21, 31,
                // ローパスフィルタ設定
                true, 89, 8, 99,
                // ハイパスフィルタ設定 (default)
                false, 26, 6, 99,
                // コーラス設定
                true, 25, 50, 99
            }, // effects
            {
                4,      // wave (Sine)
                55,     // speed
                20,     // delay
                12,     // pm_depth
                0,      // am_depth
                2       // pitch_mod_sens
            },  // lfo
            { 70, -12, 7 }  // master (level=70, transpose=-12, feedback=7)
        },
    };
};
