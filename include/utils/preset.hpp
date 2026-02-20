#pragma once

#include <cstdint>
#include <cmath>
#include <array>
#include "types.hpp"

// プリセット最大数
constexpr uint8_t MAX_PRESETS = 39;

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
                    99, 99, 99, 80, // rate1,2,3,4
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
                    99, 99, 99, 80, // rate1,2,3,4
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
                    99, 99, 99, 80, // rate1,2,3,4
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
                    99, 99, 99, 80, // rate1,2,3,4
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
                    99, 40, 20, 80, // rate1,2,3,4
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
                    99, 40, 20, 80, // rate1,2,3,4
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
                    99, 40, 20, 80, // rate1,2,3,4
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
                    99, 40, 20, 80, // rate1,2,3,4
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
                    99, 40, 20, 80, // rate1,2,3,4
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
                    99, 40, 20, 80, // rate1,2,3,4
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

        // --- Preset 10: BRASS 1 ---
        // DX7 ROM1A #1 "BRASS   1"
        // Algorithm 22 (index 21)
        {
            "BRASS 1",
            21,             // algorithm_id (algo 22)
            {{
                // Operator 1 (OP1)
                {
                    0, 98, 0.0f, 0.0f, 7, false,
                    72, 76, 99, 71,  // rate
                    99, 88, 96, 0,   // level
                    0,               // rate_scaling
                    39, 0, 14, 3, 3, // KLS: bp=39, ld=0, rd=14, lc=+LN, rc=+LN
                    0,               // velocity_sens (KVS=0)
                    true
                },
                // Operator 2 (OP2)
                {
                    0, 86, 0.0f, 0.0f, 7, false,
                    62, 51, 29, 71,
                    82, 95, 96, 0,
                    0,
                    27, 0, 7, 3, 1,  // KLS: bp=27, ld=0, rd=7, lc=+LN, rc=-EX
                    0,
                    true
                },
                // Operator 3 (OP3)
                {
                    0, 99, 1.0f, 0.0f, -2, false,
                    77, 76, 82, 71,
                    99, 98, 98, 0,
                    0,
                    39, 0, 0, 3, 3,
                    2,
                    true
                },
                // Operator 4 (OP4)
                {
                    0, 99, 1.0f, 0.0f, 0, false,
                    77, 36, 41, 71,
                    99, 98, 98, 0,
                    0,
                    39, 0, 0, 3, 3,
                    2,
                    true
                },
                // Operator 5 (OP5)
                {
                    0, 98, 1.0f, 0.0f, 1, false,
                    77, 36, 41, 71,
                    99, 98, 98, 0,
                    0,
                    39, 0, 0, 3, 3,
                    2,
                    true
                },
                // Operator 6 (OP6)
                {
                    0, 82, 1.0f, 0.0f, 0, false,
                    49, 99, 28, 68,
                    98, 98, 91, 0,
                    4,
                    39, 54, 50, 1, 1, // KLS: bp=39, ld=54, rd=50, lc=-EX, rc=-EX
                    2,
                    true
                }
            }},
            {},  // effects
            {
                4, 37, 0,   // wave=Sine, speed=37, delay=0
                5, 0,        // pm_depth=5, am_depth=0
                3,            // pitch_mod_sens=3
                false         // key_sync=false
            },  // lfo
            { 70, 0, 7 }  // master: level=70, transpose=0, feedback=7
        },

        // --- ROM1A #2: BRASS 2 ---
        {
            "BRASS 2",
            21,             // algorithm_id (algo 22)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 0.0f, 0.0f, 7, false,
                    99, 39, 32, 71,  // rate
                    99, 98, 80, 0,  // level
                    0,               // rate_scaling
                    51, 0, 38, 3, 3,  // KLS: bp=51, ld=0, rd=38, lc=3, rc=3
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 84, 0.0f, 0.0f, 7, false,
                    99, 39, 32, 71,  // rate
                    99, 98, 80, 0,  // level
                    0,               // rate_scaling
                    51, 0, 38, 3, 3,  // KLS: bp=51, ld=0, rd=38, lc=3, rc=3
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 99, 0.0f, 0.0f, -3, false,
                    99, 39, 32, 71,  // rate
                    99, 98, 81, 0,  // level
                    0,               // rate_scaling
                    39, 0, 0, 3, 3,  // KLS: bp=39, ld=0, rd=0, lc=3, rc=3
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 99, 0.0f, 0.0f, -2, false,
                    99, 39, 32, 71,  // rate
                    99, 98, 81, 0,  // level
                    0,               // rate_scaling
                    39, 0, 0, 3, 3,  // KLS: bp=39, ld=0, rd=0, lc=3, rc=3
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 99, 0.0f, 0.0f, 1, false,
                    99, 39, 32, 71,  // rate
                    99, 98, 81, 0,  // level
                    0,               // rate_scaling
                    39, 0, 0, 3, 3,  // KLS: bp=39, ld=0, rd=0, lc=3, rc=3
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 80, 0.0f, 0.0f, 0, false,
                    99, 39, 32, 71,  // rate
                    99, 98, 88, 0,  // level
                    0,               // rate_scaling
                    51, 0, 0, 3, 3,  // KLS: bp=51, ld=0, rd=0, lc=3, rc=3
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                4, 37, 0,   // wave, speed, delay
                0, 0,        // pm_depth, am_depth
                3,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, 0, 7 }  // master: level=70, transpose=0, feedback=7
        },
        // --- ROM1A #3: BRASS 3 ---
        {
            "BRASS 3",
            17,             // algorithm_id (algo 18)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 1.0f, 0.0f, 0, false,
                    55, 24, 19, 55,  // rate
                    99, 86, 86, 0,  // level
                    2,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 70, 1.0f, 0.0f, 0, false,
                    37, 34, 15, 70,  // rate
                    85, 0, 0, 0,  // level
                    2,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 77, 1.0f, 0.0f, 0, false,
                    46, 35, 22, 50,  // rate
                    99, 86, 86, 0,  // level
                    1,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 79, 1.0f, 0.0f, 0, false,
                    66, 92, 22, 50,  // rate
                    53, 61, 62, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 70, 3.0f, 6.0f, -1, false,
                    48, 55, 22, 50,  // rate
                    98, 61, 62, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 79, 7.0f, 21.0f, 0, false,
                    77, 56, 20, 70,  // rate
                    99, 0, 0, 0,  // level
                    7,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                0, 35, 0,   // wave, speed, delay
                5, 0,        // pm_depth, am_depth
                3,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, -12, 6 }  // master: level=70, transpose=-12, feedback=6
        },
        // --- Preset 11: STRINGS 1 ---
        // DX7 ROM1A #4 "STRINGS 1"
        // Algorithm 2 (index 1): [1]->[0], [3]->[2], [5*]->[4]
        {
            "STRINGS 1",
            1,              // algorithm_id (algo 2)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 1.0f, 0.0f, 0, false,
                    45, 24, 20, 41,  // rate
                    99, 85, 70, 0,   // level
                    2,               // rate_scaling
                    0, 0, 0, 0, 0,   // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    3,               // velocity_sens (KVS)
                    true
                },
                // Operator 2 (OP2)
                {
                    0, 83, 1.0f, 0.0f, 0, false,
                    75, 71, 17, 49,
                    82, 92, 62, 0,
                    1,
                    54, 0, 0, 0, 0,
                    0,
                    true
                },
                // Operator 3 (OP3)
                {
                    0, 86, 1.0f, 0.0f, 0, false,
                    44, 45, 20, 54,
                    99, 85, 82, 0,
                    0,
                    56, 0, 97, 0, 0,
                    7,
                    true
                },
                // Operator 4 (OP4)
                {
                    0, 77, 1.0f, 0.0f, 0, false,
                    96, 19, 20, 54,
                    99, 92, 86, 0,
                    2,
                    0, 0, 0, 0, 0,
                    2,
                    true
                },
                // Operator 5 (OP5)
                {
                    0, 84, 3.0f, 0.0f, 0, false,
                    53, 19, 20, 54,
                    86, 92, 86, 0,
                    2,
                    0, 0, 0, 0, 0,
                    2,
                    true
                },
                // Operator 6 (OP6)
                {
                    0, 53, 14.0f, 0.0f, 0, false,
                    53, 19, 20, 54,
                    99, 92, 86, 0,
                    2,
                    0, 0, 0, 0, 0,
                    2,
                    true
                }
            }},
            {},  // effects
            {
                0, 30, 0,   // wave=Triangle, speed=30, delay=0
                8, 0,        // pm_depth=8, am_depth=0
                2,            // pitch_mod_sens=2
                false         // key_sync=false
            },  // lfo
            { 70, 0, 7 }  // master: level=70, transpose=0, feedback=7
        },

        // --- ROM1A #5: STRINGS 2 ---
        {
            "STRINGS 2",
            1,             // algorithm_id (algo 2)
            {{
                // Operator 1 (OP1)
                {
                    0, 92, 2.0f, 0.0f, 0, false,
                    48, 56, 10, 47,  // rate
                    98, 98, 36, 0,  // level
                    0,               // rate_scaling
                    98, 0, 0, 0, 0,  // KLS: bp=98, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 74, 2.0f, 0.0f, -6, false,
                    81, 13, 7, 25,  // rate
                    99, 92, 28, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 92, 2.0f, 0.0f, 6, false,
                    51, 15, 10, 47,  // rate
                    99, 92, 0, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 76, 2.0f, 0.0f, 0, false,
                    49, 74, 10, 32,  // rate
                    98, 98, 36, 0,  // level
                    0,               // rate_scaling
                    98, 0, 0, 0, 0,  // KLS: bp=98, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 66, 2.0f, 0.0f, 0, false,
                    76, 73, 10, 28,  // rate
                    99, 92, 0, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 70, 8.0f, 0.0f, 0, false,
                    72, 76, 10, 32,  // rate
                    99, 92, 0, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                4, 30, 81,   // wave, speed, delay
                8, 0,        // pm_depth, am_depth
                2,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, -12, 7 }  // master: level=70, transpose=-12, feedback=7
        },
        // --- ROM1A #6: STRINGS 3 ---
        {
            "STRINGS 3",
            14,             // algorithm_id (algo 15)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 1.0f, 0.0f, 0, false,
                    52, 30, 25, 43,  // rate
                    99, 92, 90, 0,  // level
                    2,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 86, 1.0f, 0.0f, 0, false,
                    99, 71, 35, 51,  // rate
                    82, 92, 87, 0,  // level
                    1,               // rate_scaling
                    54, 0, 0, 0, 0,  // KLS: bp=54, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 99, 1.0f, 0.0f, 0, false,
                    50, 52, 35, 41,  // rate
                    99, 92, 91, 0,  // level
                    2,               // rate_scaling
                    51, 98, 60, 3, 0,  // KLS: bp=51, ld=98, rd=60, lc=3, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 75, 1.0f, 0.0f, 0, false,
                    96, 19, 20, 54,  // rate
                    99, 92, 89, 0,  // level
                    2,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 84, 3.0f, 0.0f, 0, false,
                    53, 67, 38, 54,  // rate
                    86, 92, 74, 0,  // level
                    2,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 54, 14.0f, 0.0f, 0, false,
                    53, 64, 44, 54,  // rate
                    99, 92, 56, 0,  // level
                    2,               // rate_scaling
                    55, 25, 0, 3, 0,  // KLS: bp=55, ld=25, rd=0, lc=3, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                4, 28, 46,   // wave, speed, delay
                30, 0,        // pm_depth, am_depth
                1,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, -12, 7 }  // master: level=70, transpose=-12, feedback=7
        },
        // --- ROM1A #7: ORCHESTRA ---
        {
            "ORCHESTRA",
            1,             // algorithm_id (algo 2)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 1.0f, 0.0f, 0, false,
                    80, 56, 10, 45,  // rate
                    98, 98, 36, 0,  // level
                    0,               // rate_scaling
                    98, 0, 0, 0, 0,  // KLS: bp=98, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 83, 1.0f, 0.0f, -6, false,
                    53, 46, 32, 61,  // rate
                    99, 93, 90, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 96, 2.0f, 0.0f, 6, false,
                    54, 15, 10, 47,  // rate
                    99, 92, 0, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 72, 2.0f, 0.0f, 0, false,
                    56, 74, 10, 45,  // rate
                    98, 98, 36, 0,  // level
                    0,               // rate_scaling
                    98, 0, 0, 0, 0,  // KLS: bp=98, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 80, 2.0f, 0.0f, 0, false,
                    76, 73, 10, 55,  // rate
                    99, 92, 0, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 82, 2.0f, 0.0f, 0, false,
                    72, 76, 10, 32,  // rate
                    99, 92, 0, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                4, 30, 63,   // wave, speed, delay
                6, 0,        // pm_depth, am_depth
                3,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, -12, 7 }  // master: level=70, transpose=-12, feedback=7
        },
        // --- ROM1A #8: PIANO 1 ---
        {
            "PIANO 1",
            18,             // algorithm_id (algo 19)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 1.0f, 0.0f, -2, false,
                    81, 25, 20, 48,  // rate
                    99, 82, 0, 0,  // level
                    4,               // rate_scaling
                    0, 85, 0, 3, 0,  // KLS: bp=0, ld=85, rd=0, lc=3, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 87, 1.0f, 0.0f, 2, false,
                    99, 0, 25, 0,  // rate
                    99, 75, 0, 0,  // level
                    5,               // rate_scaling
                    0, 0, 13, 0, 0,  // KLS: bp=0, ld=0, rd=13, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 57, 3.0f, 0.0f, 0, false,
                    81, 25, 25, 14,  // rate
                    99, 99, 99, 0,  // level
                    5,               // rate_scaling
                    47, 32, 74, 3, 0,  // KLS: bp=47, ld=32, rd=74, lc=3, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 99, 1.0f, 0.0f, 1, false,
                    81, 23, 22, 45,  // rate
                    99, 78, 0, 0,  // level
                    5,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 93, 1.0f, 58.0f, -1, false,
                    81, 58, 36, 39,  // rate
                    99, 14, 0, 0,  // level
                    5,               // rate_scaling
                    48, 0, 66, 0, 0,  // KLS: bp=48, ld=0, rd=66, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 82, 1.0f, 0.0f, -1, false,
                    99, 0, 25, 0,  // rate
                    99, 75, 0, 0,  // level
                    5,               // rate_scaling
                    0, 0, 10, 0, 0,  // KLS: bp=0, ld=0, rd=10, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                0, 35, 0,   // wave, speed, delay
                0, 0,        // pm_depth, am_depth
                4,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, 0, 6 }  // master: level=70, transpose=0, feedback=6
        },
        // --- ROM1A #9: PIANO 2 ---
        {
            "PIANO 2",
            17,             // algorithm_id (algo 18)
            {{
                // Operator 1 (OP1)
                {
                    0, 94, 1.0f, 0.0f, 0, false,
                    80, 24, 10, 50,  // rate
                    99, 62, 0, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 86, 1.0f, 0.0f, 2, false,
                    95, 0, 25, 0,  // rate
                    99, 75, 0, 0,  // level
                    2,               // rate_scaling
                    0, 0, 10, 0, 0,  // KLS: bp=0, ld=0, rd=10, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 83, 5.0f, 0.0f, 1, false,
                    90, 27, 20, 50,  // rate
                    99, 85, 0, 0,  // level
                    5,               // rate_scaling
                    32, 0, 27, 0, 0,  // KLS: bp=32, ld=0, rd=27, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 84, 1.0f, 0.0f, -1, false,
                    97, 27, 10, 25,  // rate
                    99, 86, 48, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 94, 0.0f, 0.0f, 0, false,
                    90, 71, 33, 31,  // rate
                    99, 0, 0, 0,  // level
                    3,               // rate_scaling
                    27, 0, 26, 2, 0,  // KLS: bp=27, ld=0, rd=26, lc=2, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 78, 0.0f, 0.0f, 1, false,
                    92, 71, 58, 36,  // rate
                    99, 0, 0, 0,  // level
                    3,               // rate_scaling
                    36, 0, 98, 0, 0,  // KLS: bp=36, ld=0, rd=98, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                0, 30, 0,   // wave, speed, delay
                0, 0,        // pm_depth, am_depth
                4,            // pitch_mod_sens
                false,         // key_sync
                false          // osc_key_sync
            },  // lfo
            { 70, -12, 5 }  // master: level=70, transpose=-12, feedback=5
        },
        // --- ROM1A #10: PIANO 3 ---
        {
            "PIANO 3",
            2,             // algorithm_id (algo 3)
            {{
                // Operator 1 (OP1)
                {
                    0, 86, 1.0f, 0.0f, -4, false,
                    90, 30, 28, 45,  // rate
                    99, 95, 0, 0,  // level
                    3,               // rate_scaling
                    32, 0, 0, 0, 0,  // KLS: bp=32, ld=0, rd=0, lc=0, rc=0
                    3,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 85, 1.0f, 0.0f, 4, false,
                    98, 36, 6, 32,  // rate
                    91, 90, 0, 0,  // level
                    2,               // rate_scaling
                    50, 22, 50, 3, 0,  // KLS: bp=50, ld=22, rd=50, lc=3, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 97, 7.0f, 0.0f, -3, false,
                    94, 80, 19, 12,  // rate
                    83, 67, 0, 0,  // level
                    3,               // rate_scaling
                    43, 9, 20, 3, 0,  // KLS: bp=43, ld=9, rd=20, lc=3, rc=0
                    3,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 95, 1.0f, 0.0f, 3, false,
                    90, 64, 28, 45,  // rate
                    99, 97, 0, 0,  // level
                    3,               // rate_scaling
                    46, 0, 0, 0, 0,  // KLS: bp=46, ld=0, rd=0, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 87, 1.0f, 0.0f, -2, false,
                    98, 20, 6, 2,  // rate
                    91, 90, 0, 0,  // level
                    2,               // rate_scaling
                    41, 0, 27, 3, 0,  // KLS: bp=41, ld=0, rd=27, lc=3, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 84, 0.0f, 0.0f, 2, false,
                    80, 73, 15, 10,  // rate
                    99, 19, 0, 0,  // level
                    3,               // rate_scaling
                    53, 0, 0, 0, 3,  // KLS: bp=53, ld=0, rd=0, lc=0, rc=3
                    5,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                0, 45, 0,   // wave, speed, delay
                0, 0,        // pm_depth, am_depth
                4,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, 0, 4 }  // master: level=70, transpose=0, feedback=4
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
                    96, 25, 25, 37, // rate1,2,3,4
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
                    95, 20, 20, 37, // rate1,2,3,4
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
                    95, 20, 20, 37, // rate1,2,3,4
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
            {
                // ディレイ設定
                true, 80, 30, 50,
                // ローパスフィルタ設定
                false, 89, 8, 99,
                // ハイパスフィルタ設定 (default)
                false, 26, 6, 99,
                // コーラス設定
                true, 20, 50, 50,
                // リバーブ設定
                true, 50, 50, 25
            },
            {}, // lfo (default)
            { 70, 0, 6 }  // master (level=70, feedback=6)
        },

        // --- ROM1A #12: GUITAR 1 ---
        {
            "GUITAR 1",
            7,             // algorithm_id (algo 8)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 1.0f, 0.0f, 0, false,
                    74, 85, 27, 70,  // rate
                    99, 95, 0, 0,  // level
                    4,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    5,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 93, 3.0f, 0.0f, 0, false,
                    91, 25, 39, 60,  // rate
                    99, 86, 0, 0,  // level
                    2,               // rate_scaling
                    0, 0, 65, 0, 0,  // KLS: bp=0, ld=0, rd=65, lc=0, rc=0
                    7,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 99, 1.0f, 0.0f, 0, false,
                    78, 87, 22, 75,  // rate
                    99, 92, 0, 0,  // level
                    3,               // rate_scaling
                    34, 9, 0, 0, 0,  // KLS: bp=34, ld=9, rd=0, lc=0, rc=0
                    7,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 89, 3.0f, 0.0f, 0, false,
                    81, 87, 22, 75,  // rate
                    99, 92, 0, 0,  // level
                    4,               // rate_scaling
                    0, 0, 14, 0, 0,  // KLS: bp=0, ld=0, rd=14, lc=0, rc=0
                    4,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 99, 3.0f, 0.0f, 0, false,
                    81, 87, 22, 75,  // rate
                    99, 92, 0, 0,  // level
                    4,               // rate_scaling
                    0, 0, 15, 0, 0,  // KLS: bp=0, ld=0, rd=15, lc=0, rc=0
                    7,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 57, 12.0f, 0.0f, 0, false,
                    99, 57, 99, 75,  // rate
                    99, 0, 0, 0,  // level
                    0,               // rate_scaling
                    39, 53, 20, 0, 0,  // KLS: bp=39, ld=53, rd=20, lc=0, rc=0
                    6,               // velocity_sens
                    true,
                    3                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                4, 35, 0,   // wave, speed, delay
                1, 3,        // pm_depth, am_depth
                3,            // pitch_mod_sens
                false,         // key_sync
                false          // osc_key_sync
            },  // lfo
            { 70, 0, 7 }  // master: level=70, transpose=0, feedback=7
        },
        // --- ROM1A #13: GUITAR 2 ---
        {
            "GUITAR 2",
            15,             // algorithm_id (algo 16)
            {{
                // Operator 1 (OP1)
                {
                    0, 86, 3.0f, 0.0f, 0, false,
                    95, 67, 99, 71,  // rate
                    99, 99, 99, 0,  // level
                    0,               // rate_scaling
                    0, 82, 0, 3, 3,  // KLS: bp=0, ld=82, rd=0, lc=3, rc=3
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 87, 1.0f, 0.0f, 0, false,
                    99, 99, 99, 42,  // rate
                    99, 99, 99, 99,  // level
                    1,               // rate_scaling
                    48, 0, 0, 3, 0,  // KLS: bp=48, ld=0, rd=0, lc=3, rc=0
                    7,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 99, 1.0f, 50.0f, 0, false,
                    99, 99, 99, 71,  // rate
                    99, 99, 99, 0,  // level
                    0,               // rate_scaling
                    39, 0, 40, 3, 0,  // KLS: bp=39, ld=0, rd=40, lc=3, rc=0
                    7,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 70, 0.0f, 0.0f, 0, false,
                    92, 99, 15, 71,  // rate
                    99, 96, 75, 0,  // level
                    0,               // rate_scaling
                    60, 0, 0, 3, 0,  // KLS: bp=60, ld=0, rd=0, lc=3, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 85, 0.0f, 0.0f, 0, false,
                    99, 99, 12, 0,  // rate
                    99, 99, 76, 0,  // level
                    0,               // rate_scaling
                    60, 0, 0, 3, 0,  // KLS: bp=60, ld=0, rd=0, lc=3, rc=0
                    7,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 73, 3.0f, 0.0f, 0, false,
                    99, 44, 1, 71,  // rate
                    99, 96, 75, 0,  // level
                    0,               // rate_scaling
                    60, 0, 46, 3, 0,  // KLS: bp=60, ld=0, rd=46, lc=3, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                0, 35, 0,   // wave, speed, delay
                0, 0,        // pm_depth, am_depth
                4,            // pitch_mod_sens
                false,         // key_sync
                false          // osc_key_sync
            },  // lfo
            { 70, 0, 7 }  // master: level=70, transpose=0, feedback=7
        },
        // --- ROM1A #14: SYN-LEAD 1 ---
        {
            "SYN-LEAD 1",
            17,             // algorithm_id (algo 18)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 1.0f, 0.0f, 1, false,
                    99, 0, 12, 70,  // rate
                    99, 95, 95, 0,  // level
                    1,               // rate_scaling
                    32, 0, 0, 0, 0,  // KLS: bp=32, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 71, 1.0f, 0.0f, -1, false,
                    99, 95, 0, 0,  // rate
                    99, 96, 89, 0,  // level
                    3,               // rate_scaling
                    32, 0, 0, 0, 0,  // KLS: bp=32, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 82, 1.0f, 0.0f, 0, false,
                    99, 87, 0, 0,  // rate
                    93, 90, 0, 0,  // level
                    3,               // rate_scaling
                    32, 0, 21, 0, 0,  // KLS: bp=32, ld=0, rd=21, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 71, 2.0f, 0.0f, 2, false,
                    99, 92, 28, 60,  // rate
                    99, 90, 0, 0,  // level
                    6,               // rate_scaling
                    48, 0, 60, 0, 0,  // KLS: bp=48, ld=0, rd=60, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 43, 3.0f, 0.0f, -2, false,
                    99, 99, 97, 0,  // rate
                    99, 65, 60, 0,  // level
                    1,               // rate_scaling
                    32, 0, 0, 0, 0,  // KLS: bp=32, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 47, 2.0f, 0.0f, 0, true,
                    99, 70, 60, 0,  // rate
                    99, 99, 97, 0,  // level
                    3,               // rate_scaling
                    32, 0, 21, 0, 0,  // KLS: bp=32, ld=0, rd=21, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                4, 37, 42,   // wave, speed, delay
                0, 99,        // pm_depth, am_depth
                4,            // pitch_mod_sens
                false,         // key_sync
                false          // osc_key_sync
            },  // lfo
            { 70, 12, 7 }  // master: level=70, transpose=12, feedback=7
        },
        // --- Preset 12: BASS 1 ---
        // DX7 ROM1A #15 "BASS    1"
        // Algorithm 16 (index 15)
        {
            "BASS 1",
            15,             // algorithm_id (algo 16)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 0.0f, 0.0f, 0, false,
                    95, 62, 17, 58,  // rate
                    99, 95, 32, 0,   // level
                    7,               // rate_scaling
                    36, 57, 14, 3, 0,// KLS: bp=36, ld=57, rd=14, lc=-LN→3, rc=-LN→0
                    0,               // velocity_sens
                    true
                },
                // Operator 2 (OP2)
                {
                    0, 80, 0.0f, 0.0f, 0, false,
                    99, 20, 0, 0,
                    99, 0, 0, 0,
                    7,
                    41, 0, 0, 0, 0,
                    0,
                    true
                },
                // Operator 3 (OP3)
                {
                    0, 99, 0.0f, 0.0f, 0, false,
                    88, 96, 32, 30,
                    79, 65, 0, 0,
                    6,
                    0, 0, 0, 0, 0,
                    3,
                    true
                },
                // Operator 4 (OP4)
                {
                    0, 93, 5.0f, 0.0f, 0, false,
                    90, 42, 7, 55,
                    90, 30, 0, 0,
                    5,
                    0, 0, 0, 0, 0,
                    5,
                    true
                },
                // Operator 5 (OP5)
                {
                    0, 62, 0.0f, 0.0f, 0, false,
                    99, 0, 0, 0,
                    99, 0, 0, 0,
                    7,
                    52, 75, 0, 0, 0,
                    3,
                    true
                },
                // Operator 6 (OP6)
                {
                    0, 85, 9.0f, 0.0f, 0, false,
                    94, 56, 24, 55,
                    93, 28, 0, 0,
                    1,
                    0, 0, 0, 0, 0,
                    7,
                    true
                }
            }},
            {},  // effects
            {
                0, 35, 0,   // wave=Triangle, speed=35, delay=0
                0, 0,        // pm_depth=0, am_depth=0
                3,            // pitch_mod_sens=3
                false         // key_sync=false
            },  // lfo
            { 100, 0, 7 }  // master: level=70, transpose=0, feedback=7
        },

        // --- ROM1A #16: BASS 2 ---
        {
            "BASS 2",
            16,             // algorithm_id (algo 17)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 0.0f, 1.0f, 0, false,
                    75, 37, 18, 63,  // rate
                    99, 70, 0, 0,  // level
                    3,               // rate_scaling
                    48, 0, 32, 0, 0,  // KLS: bp=48, ld=0, rd=32, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 80, 0.0f, 3.0f, 0, false,
                    28, 37, 42, 50,  // rate
                    99, 0, 0, 0,  // level
                    1,               // rate_scaling
                    41, 0, 35, 0, 0,  // KLS: bp=41, ld=0, rd=35, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 68, 1.0f, 0.0f, 7, false,
                    73, 25, 32, 30,  // rate
                    97, 78, 0, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    3,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 99, 0.0f, 0.0f, 0, false,
                    80, 39, 28, 53,  // rate
                    93, 57, 0, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 75, 1.0f, 1.0f, 0, false,
                    99, 51, 0, 0,  // rate
                    99, 74, 0, 0,  // level
                    4,               // rate_scaling
                    34, 0, 32, 0, 0,  // KLS: bp=34, ld=0, rd=32, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 87, 0.0f, 0.0f, 1, false,
                    25, 50, 24, 55,  // rate
                    96, 97, 0, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    7,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                4, 31, 33,   // wave, speed, delay
                0, 0,        // pm_depth, am_depth
                2,            // pitch_mod_sens
                false,         // key_sync
                false          // osc_key_sync
            },  // lfo
            { 70, -12, 7 }  // master: level=70, transpose=-12, feedback=7
        },
        // --- Preset 14: E.ORGAN 1 ---
        // DX7 ROM1A #17 "E.ORGAN 1"
        // Algorithm 32 (index 31): 全OP独立出力
        {
            "E.ORGAN 1",
            31,             // algorithm_id (algo 32)
            {{
                // Operator 1 (OP1)
                {
                    0, 94, 0.0f, 0.0f, -2, false,
                    99, 80, 22, 90,  // rate
                    99, 99, 99, 0,   // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,   // KLS
                    0,               // velocity_sens
                    true
                },
                // Operator 2 (OP2)
                {
                    0, 94, 1.0f, 1.0f, -6, false,
                    99, 20, 22, 90,
                    99, 99, 97, 0,
                    0,
                    0, 0, 10, 0, 0,
                    0,
                    true
                },
                // Operator 3 (OP3)
                {
                    0, 94, 1.0f, 50.0f, 4, false,
                    99, 80, 54, 82,
                    99, 99, 99, 0,
                    0,
                    0, 0, 0, 0, 0,
                    0,
                    true
                },
                // Operator 4 (OP4)
                {
                    0, 94, 0.0f, 0.0f, 5, false,
                    99, 80, 22, 90,
                    99, 99, 99, 0,
                    0,
                    0, 0, 0, 0, 0,
                    0,
                    true
                },
                // Operator 5 (OP5)
                {
                    0, 94, 1.0f, 0.0f, 2, false,
                    99, 80, 22, 90,
                    99, 99, 99, 0,
                    0,
                    0, 0, 0, 0, 0,
                    0,
                    true
                },
                // Operator 6 (OP6)
                {
                    0, 94, 3.0f, 0.0f, 0, false,
                    99, 54, 22, 90,
                    99, 0, 0, 0,
                    0,
                    0, 0, 0, 0, 0,
                    0,
                    true
                }
            }},
            {
                // ディレイ設定
                false, 80, 30, 50,
                // ローパスフィルタ設定
                false, 89, 8, 99,
                // ハイパスフィルタ設定 (default)
                false, 26, 6, 99,
                // コーラス設定
                true, 20, 50, 50,
                // リバーブ設定
                false, 50, 50, 25
            },
            {
                0, 35, 0,   // wave=Triangle, speed=35, delay=0
                0, 0,        // pm_depth=0, am_depth=0
                4,            // pitch_mod_sens=4
                false         // key_sync=false
            },  // lfo
            { 70, 0, 0 }  // master: level=70, transpose=0, feedback=0
        },

        // --- ROM1A #18: PIPES 1 ---
        {
            "PIPES 1",
            18,             // algorithm_id (algo 19)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 0.0f, 0.0f, 0, false,
                    45, 25, 25, 36,  // rate
                    99, 99, 98, 0,  // level
                    5,               // rate_scaling
                    41, 0, 50, 0, 0,  // KLS: bp=41, ld=0, rd=50, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 90, 0.0f, 0.0f, 0, false,
                    99, 97, 62, 47,  // rate
                    99, 99, 90, 0,  // level
                    4,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 75, 1.0f, 0.0f, 0, false,
                    99, 97, 62, 47,  // rate
                    99, 99, 90, 0,  // level
                    5,               // rate_scaling
                    46, 17, 40, 3, 0,  // KLS: bp=46, ld=17, rd=40, lc=3, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 88, 4.0f, 0.0f, 0, false,
                    61, 25, 25, 50,  // rate
                    99, 99, 97, 0,  // level
                    3,               // rate_scaling
                    60, 10, 10, 0, 0,  // KLS: bp=60, ld=10, rd=10, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 97, 2.0f, 0.0f, 0, false,
                    61, 25, 25, 61,  // rate
                    99, 99, 93, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 76, 10.0f, 0.0f, 0, false,
                    72, 25, 25, 70,  // rate
                    99, 99, 99, 0,  // level
                    3,               // rate_scaling
                    46, 10, 1, 0, 3,  // KLS: bp=46, ld=10, rd=1, lc=0, rc=3
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                4, 34, 33,   // wave, speed, delay
                0, 0,        // pm_depth, am_depth
                2,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, -12, 7 }  // master: level=70, transpose=-12, feedback=7
        },
        // --- ROM1A #19: HARPSICH 1 ---
        {
            "HARPSICH 1",
            4,             // algorithm_id (algo 5)
            {{
                // Operator 1 (OP1)
                {
                    0, 89, 4.0f, 0.0f, 0, false,
                    95, 28, 27, 47,  // rate
                    99, 90, 0, 0,  // level
                    3,               // rate_scaling
                    49, 0, 0, 0, 0,  // KLS: bp=49, ld=0, rd=0, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 99, 0.0f, 0.0f, 0, false,
                    95, 72, 71, 99,  // rate
                    99, 97, 91, 98,  // level
                    1,               // rate_scaling
                    49, 0, 0, 0, 0,  // KLS: bp=49, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 85, 1.0f, 0.0f, -1, false,
                    95, 28, 27, 47,  // rate
                    99, 90, 0, 0,  // level
                    3,               // rate_scaling
                    49, 0, 0, 0, 0,  // KLS: bp=49, ld=0, rd=0, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 99, 3.0f, 0.0f, 0, false,
                    95, 72, 71, 99,  // rate
                    99, 97, 91, 98,  // level
                    1,               // rate_scaling
                    64, 0, 46, 0, 0,  // KLS: bp=64, ld=0, rd=46, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 83, 4.0f, 0.0f, -1, false,
                    95, 28, 27, 47,  // rate
                    99, 90, 0, 0,  // level
                    3,               // rate_scaling
                    49, 0, 0, 0, 0,  // KLS: bp=49, ld=0, rd=0, lc=0, rc=0
                    3,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 87, 6.0f, 0.0f, 0, false,
                    95, 72, 71, 99,  // rate
                    99, 97, 91, 98,  // level
                    1,               // rate_scaling
                    64, 0, 55, 0, 0,  // KLS: bp=64, ld=0, rd=55, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                0, 35, 0,   // wave, speed, delay
                0, 0,        // pm_depth, am_depth
                2,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, 0, 1 }  // master: level=70, transpose=0, feedback=1
        },
        // --- Preset 15: CLAV 1 ---
        // DX7 ROM1A #20 "CLAV    1"
        // Algorithm 3 (index 2)
        {
            "CLAV 1",
            2,              // algorithm_id (algo 3)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 0.0f, 0.0f, 1, false,
                    95, 92, 28, 60,  // rate
                    99, 90, 0, 0,    // level
                    3,               // rate_scaling
                    32, 0, 0, 0, 0,  // KLS
                    3,               // velocity_sens
                    true
                },
                // Operator 2 (OP2)
                {
                    0, 99, 0.0f, 0.0f, -1, false,
                    95, 95, 0, 0,
                    99, 96, 89, 0,
                    3,
                    32, 0, 0, 0, 0,
                    1,
                    true
                },
                // Operator 3 (OP3)
                {
                    0, 71, 4.0f, 50.0f, 0, false,
                    98, 87, 0, 0,
                    87, 86, 0, 0,
                    3,
                    32, 0, 21, 0, 0,
                    1,
                    true
                },
                // Operator 4 (OP4)
                {
                    0, 99, 2.0f, 0.0f, 0, false,
                    95, 92, 28, 60,
                    99, 90, 0, 0,
                    3,
                    32, 0, 0, 0, 0,
                    2,
                    true
                },
                // Operator 5 (OP5)
                {
                    0, 99, 0.0f, 0.0f, -2, false,
                    95, 95, 0, 0,
                    99, 96, 89, 0,
                    3,
                    32, 0, 0, 0, 0,
                    6,
                    true
                },
                // Operator 6 (OP6)
                {
                    0, 78, 8.0f, 0.0f, 0, false,
                    98, 87, 0, 0,
                    87, 86, 0, 0,
                    3,
                    32, 0, 21, 0, 0,
                    7,
                    true
                }
            }},
            {},  // effects
            {
                4, 30, 0,   // wave=Sine, speed=30, delay=0
                0, 0,        // pm_depth=0, am_depth=0
                2,            // pitch_mod_sens=2
                false         // key_sync=false
            },  // lfo
            { 70, 0, 5 }  // master: level=70, transpose=0, feedback=5
        },

        // --- ROM1A #21: VIBE 1 ---
        {
            "VIBE 1",
            22,             // algorithm_id (algo 23)
            {{
                // Operator 1 (OP1)
                {
                    0, 50, 4.0f, 0.0f, 0, false,
                    99, 28, 99, 50,  // rate
                    99, 25, 0, 0,  // level
                    2,               // rate_scaling
                    39, 12, 12, 0, 3,  // KLS: bp=39, ld=12, rd=12, lc=0, rc=3
                    7,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 99, 1.0f, 0.0f, 0, false,
                    80, 85, 24, 50,  // rate
                    99, 90, 0, 0,  // level
                    2,               // rate_scaling
                    39, 4, 12, 0, 3,  // KLS: bp=39, ld=4, rd=12, lc=0, rc=3
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 72, 3.0f, 0.0f, 0, false,
                    80, 85, 43, 50,  // rate
                    99, 74, 0, 0,  // level
                    4,               // rate_scaling
                    39, 12, 12, 0, 3,  // KLS: bp=39, ld=12, rd=12, lc=0, rc=3
                    4,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 99, 1.0f, 0.0f, -7, false,
                    80, 85, 24, 50,  // rate
                    99, 90, 0, 0,  // level
                    3,               // rate_scaling
                    9, 0, 0, 1, 1,  // KLS: bp=9, ld=0, rd=0, lc=1, rc=1
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 99, 1.0f, 0.0f, 7, false,
                    80, 85, 24, 50,  // rate
                    99, 90, 42, 0,  // level
                    3,               // rate_scaling
                    9, 0, 0, 1, 1,  // KLS: bp=9, ld=0, rd=0, lc=1, rc=1
                    5,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 57, 14.0f, 0.0f, 0, false,
                    99, 48, 99, 50,  // rate
                    99, 32, 0, 0,  // level
                    5,               // rate_scaling
                    39, 12, 12, 0, 3,  // KLS: bp=39, ld=12, rd=12, lc=0, rc=3
                    7,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                0, 26, 0,   // wave, speed, delay
                0, 0,        // pm_depth, am_depth
                2,            // pitch_mod_sens
                true,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, 0, 5 }  // master: level=70, transpose=0, feedback=5
        },
        // --- ROM1A #22: MARIMBA ---
        {
            "MARIMBA",
            6,             // algorithm_id (algo 7)
            {{
                // Operator 1 (OP1)
                {
                    0, 95, 0.0f, 0.0f, 0, false,
                    95, 40, 49, 55,  // rate
                    99, 92, 0, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 96, 3.0f, 0.0f, 0, false,
                    99, 72, 0, 0,  // rate
                    82, 48, 0, 0,  // level
                    0,               // rate_scaling
                    54, 0, 46, 0, 0,  // KLS: bp=54, ld=0, rd=46, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 99, 0.0f, 0.0f, 0, false,
                    95, 33, 49, 41,  // rate
                    99, 92, 0, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 85, 5.0f, 0.0f, 0, false,
                    99, 75, 0, 82,  // rate
                    82, 48, 0, 0,  // level
                    0,               // rate_scaling
                    54, 0, 46, 0, 0,  // KLS: bp=54, ld=0, rd=46, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 93, 0.0f, 50.0f, 0, false,
                    99, 75, 0, 8,  // rate
                    82, 48, 0, 0,  // level
                    0,               // rate_scaling
                    54, 0, 46, 0, 0,  // KLS: bp=54, ld=0, rd=46, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 99, 4.0f, 13.0f, 0, false,
                    0, 63, 55, 0,  // rate
                    78, 78, 0, 0,  // level
                    0,               // rate_scaling
                    41, 0, 0, 0, 0,  // KLS: bp=41, ld=0, rd=0, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                0, 35, 0,   // wave, speed, delay
                0, 0,        // pm_depth, am_depth
                3,            // pitch_mod_sens
                true,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, 0, 0 }  // master: level=70, transpose=0, feedback=0
        },
        // --- ROM1A #23: KOTO ---
        {
            "KOTO",
            1,             // algorithm_id (algo 2)
            {{
                // Operator 1 (OP1)
                {
                    0, 90, 1.0f, 0.0f, 0, false,
                    94, 62, 58, 34,  // rate
                    99, 92, 0, 0,  // level
                    6,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    3,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 99, 4.0f, 0.0f, 0, false,
                    99, 68, 28, 48,  // rate
                    99, 83, 0, 0,  // level
                    6,               // rate_scaling
                    0, 0, 10, 0, 0,  // KLS: bp=0, ld=0, rd=10, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 99, 1.0f, 0.0f, 0, false,
                    94, 64, 30, 33,  // rate
                    99, 92, 0, 0,  // level
                    5,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    3,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 82, 1.0f, 0.0f, 0, false,
                    90, 28, 17, 39,  // rate
                    99, 76, 0, 0,  // level
                    6,               // rate_scaling
                    10, 0, 17, 0, 1,  // KLS: bp=10, ld=0, rd=17, lc=0, rc=1
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 83, 4.0f, 0.0f, 0, false,
                    91, 37, 29, 29,  // rate
                    99, 90, 0, 0,  // level
                    6,               // rate_scaling
                    0, 0, 5, 0, 0,  // KLS: bp=0, ld=0, rd=5, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 81, 3.0f, 0.0f, 0, false,
                    82, 53, 37, 48,  // rate
                    99, 81, 0, 0,  // level
                    6,               // rate_scaling
                    0, 0, 5, 0, 0,  // KLS: bp=0, ld=0, rd=5, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                4, 30, 40,   // wave, speed, delay
                17, 15,        // pm_depth, am_depth
                2,            // pitch_mod_sens
                true,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, 0, 7 }  // master: level=70, transpose=0, feedback=7
        },
        // --- Preset 13: FLUTE 1 ---
        // DX7 ROM1A #24 "FLUTE   1"
        // Algorithm 16 (index 15)
        {
            "FLUTE 1",
            15,             // algorithm_id (algo 16)
            {{
                // Operator 1 (OP1)
                {
                    0, 98, 1.0f, 0.0f, -2, false,
                    61, 67, 70, 65,  // rate
                    93, 89, 98, 0,   // level
                    0,               // rate_scaling
                    41, 0, 0, 0, 0,  // KLS
                    2,               // velocity_sens
                    true
                },
                // Operator 2 (OP2)
                {
                    0, 75, 1.0f, 0.0f, 4, false,
                    99, 97, 62, 54,
                    99, 99, 90, 0,
                    4,
                    0, 0, 0, 0, 0,
                    2,
                    true
                },
                // Operator 3 (OP3)
                {
                    0, 76, 1.0f, 0.0f, -3, false,
                    53, 38, 75, 61,
                    88, 44, 24, 0,
                    0,
                    46, 0, 0, 3, 0,  // KLS: bp=46, lc=3(+LN)
                    0,               // velocity_sens
                    true,
                    1                // amp_mod_sens=1
                },
                // Operator 4 (OP4)
                {
                    0, 0, 2.0f, 0.0f, 0, false,
                    61, 25, 25, 60,
                    99, 99, 97, 0,
                    3,
                    60, 10, 10, 0, 0,
                    0,
                    true
                },
                // Operator 5 (OP5)
                {
                    0, 56, 2.0f, 0.0f, 0, false,
                    65, 38, 0, 61,
                    99, 0, 0, 0,
                    0,
                    53, 0, 43, 0, 0,
                    0,
                    true
                },
                // Operator 6 (OP6)
                {
                    0, 83, 1.0f, 53.0f, 4, false,
                    99, 64, 98, 61,
                    99, 67, 52, 0,
                    0,
                    46, 0, 0, 0, 3,  // KLS: bp=46, rc=3(+LN)
                    2,
                    true
                }
            }},
            {},  // effects
            {
                0, 30, 23,   // wave=Triangle, speed=30, delay=23
                8, 13,        // pm_depth=8, am_depth=13
                1,            // pitch_mod_sens=1
                false, false  // key_sync=false, osc_key_sync=false
            },  // lfo
            { 70, 0, 5 }  // master: level=70, transpose=0, feedback=5
        },

        // --- ROM1A #25: ORCH-CHIME ---
        {
            "ORCH-CHIME",
            4,             // algorithm_id (algo 5)
            {{
                // Operator 1 (OP1)
                {
                    0, 97, 0.0f, 0.0f, 5, false,
                    34, 42, 71, 34,  // rate
                    99, 99, 99, 0,  // level
                    3,               // rate_scaling
                    15, 0, 0, 0, 1,  // KLS: bp=15, ld=0, rd=0, lc=0, rc=1
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 87, 0.0f, 0.0f, 5, false,
                    99, 0, 0, 0,  // rate
                    99, 99, 99, 0,  // level
                    7,               // rate_scaling
                    15, 0, 0, 0, 1,  // KLS: bp=15, ld=0, rd=0, lc=0, rc=1
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 99, 0.0f, 0.0f, 0, false,
                    80, 49, 17, 30,  // rate
                    99, 95, 0, 0,  // level
                    3,               // rate_scaling
                    15, 0, 0, 0, 1,  // KLS: bp=15, ld=0, rd=0, lc=0, rc=1
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 91, 2.0f, 57.0f, 0, false,
                    80, 70, 9, 12,  // rate
                    88, 80, 0, 0,  // level
                    3,               // rate_scaling
                    15, 0, 0, 0, 1,  // KLS: bp=15, ld=0, rd=0, lc=0, rc=1
                    3,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 98, 1.0f, 0.0f, 7, false,
                    41, 42, 71, 34,  // rate
                    99, 99, 99, 0,  // level
                    3,               // rate_scaling
                    15, 0, 0, 0, 1,  // KLS: bp=15, ld=0, rd=0, lc=0, rc=1
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 75, 1.0f, 0.0f, -7, false,
                    99, 0, 0, 0,  // rate
                    99, 99, 99, 0,  // level
                    7,               // rate_scaling
                    15, 0, 0, 0, 1,  // KLS: bp=15, ld=0, rd=0, lc=0, rc=1
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                0, 30, 0,   // wave, speed, delay
                5, 0,        // pm_depth, am_depth
                3,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, 0, 7 }  // master: level=70, transpose=0, feedback=7
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

        // --- ROM1A #27: STEEL DRUM ---
        {
            "STEEL DRUM",
            14,             // algorithm_id (algo 15)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 1.0f, 0.0f, 0, false,
                    99, 40, 33, 38,  // rate
                    99, 92, 0, 0,  // level
                    4,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 64, 1.0f, 70.0f, 0, false,
                    99, 19, 20, 9,  // rate
                    99, 87, 0, 0,  // level
                    2,               // rate_scaling
                    57, 0, 71, 2, 0,  // KLS: bp=57, ld=0, rd=71, lc=2, rc=0
                    2,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 99, 1.0f, 0.0f, 0, false,
                    99, 30, 35, 42,  // rate
                    99, 92, 0, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    3,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 88, 2.0f, 0.0f, 7, false,
                    99, 44, 50, 21,  // rate
                    91, 82, 0, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 64, 4.0f, 33.0f, 0, false,
                    99, 40, 38, 0,  // rate
                    91, 82, 0, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 49, 2.0f, 60.0f, 0, true,
                    99, 49, 28, 12,  // rate
                    91, 82, 0, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                4, 25, 0,   // wave, speed, delay
                10, 99,        // pm_depth, am_depth
                2,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, 0, 5 }  // master: level=70, transpose=0, feedback=5
        },
        // --- ROM1A #28: TIMPANI ---
        {
            "TIMPANI",
            15,             // algorithm_id (algo 16)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 0.0f, 0.0f, 0, false,
                    99, 36, 98, 33,  // rate
                    99, 0, 0, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 3,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=3
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 86, 0.0f, 0.0f, 3, false,
                    99, 74, 0, 0,  // rate
                    99, 0, 0, 0,  // level
                    1,               // rate_scaling
                    41, 0, 0, 0, 1,  // KLS: bp=41, ld=0, rd=0, lc=0, rc=1
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 85, 0.0f, 36.0f, -3, false,
                    99, 77, 26, 23,  // rate
                    99, 72, 0, 0,  // level
                    3,               // rate_scaling
                    0, 0, 0, 0, 1,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=1
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 87, 0.0f, 75.0f, 0, false,
                    99, 31, 17, 30,  // rate
                    99, 75, 0, 0,  // level
                    7,               // rate_scaling
                    80, 0, 0, 3, 1,  // KLS: bp=80, ld=0, rd=0, lc=3, rc=1
                    7,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 73, 0.0f, 0.0f, 0, false,
                    99, 50, 26, 19,  // rate
                    99, 0, 0, 0,  // level
                    0,               // rate_scaling
                    80, 0, 0, 3, 1,  // KLS: bp=80, ld=0, rd=0, lc=3, rc=1
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 73, 0.0f, 56.0f, 0, false,
                    98, 2, 26, 27,  // rate
                    98, 0, 0, 0,  // level
                    3,               // rate_scaling
                    3, 0, 0, 0, 2,  // KLS: bp=3, ld=0, rd=0, lc=0, rc=2
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                0, 11, 0,   // wave, speed, delay
                16, 0,        // pm_depth, am_depth
                2,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, 0, 7 }  // master: level=70, transpose=0, feedback=7
        },
        // --- ROM1A #29: REFS WHISL ---
        {
            "REFS WHISL",
            17,             // algorithm_id (algo 18)
            {{
                // Operator 1 (OP1)
                {
                    0, 90, 3.0f, 32.0f, 0, true,
                    60, 39, 28, 49,  // rate
                    99, 99, 99, 0,  // level
                    4,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 93, 9.0f, 53.0f, 0, true,
                    60, 39, 28, 45,  // rate
                    99, 99, 99, 0,  // level
                    4,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 66, 1.0f, 67.0f, 0, true,
                    60, 39, 8, 0,  // rate
                    99, 99, 99, 0,  // level
                    4,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 75, 7.0f, 82.0f, 0, true,
                    94, 68, 24, 55,  // rate
                    96, 89, 0, 0,  // level
                    1,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 64, 4.0f, 0.0f, 0, true,
                    99, 0, 0, 0,  // rate
                    99, 0, 0, 0,  // level
                    0,               // rate_scaling
                    41, 0, 0, 0, 0,  // KLS: bp=41, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 78, 5.0f, 0.0f, 0, true,
                    94, 56, 24, 55,  // rate
                    96, 78, 0, 0,  // level
                    1,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                5, 99, 0,   // wave, speed, delay
                0, 0,        // pm_depth, am_depth
                6,            // pitch_mod_sens
                true,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, 0, 2 }  // master: level=70, transpose=0, feedback=2
        },
        // --- ROM1A #30: VOICE 1 ---
        {
            "VOICE 1",
            6,             // algorithm_id (algo 7)
            {{
                // Operator 1 (OP1)
                {
                    0, 87, 1.0f, 0.0f, -7, false,
                    34, 20, 53, 57,  // rate
                    99, 94, 97, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 99, 1.0f, 0.0f, 0, false,
                    19, 26, 53, 25,  // rate
                    51, 61, 76, 51,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    2,               // velocity_sens
                    true,
                    2                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 99, 1.0f, 0.0f, 7, false,
                    33, 20, 53, 39,  // rate
                    99, 94, 97, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    3,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 99, 1.0f, 2.0f, 3, false,
                    72, 19, 41, 12,  // rate
                    48, 58, 20, 9,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 53, 1.0f, 1.0f, -1, false,
                    35, 21, 36, 63,  // rate
                    99, 90, 85, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    1,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 55, 5.0f, 2.0f, 1, false,
                    99, 72, 48, 17,  // rate
                    99, 99, 99, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                0, 35, 35,   // wave, speed, delay
                11, 2,        // pm_depth, am_depth
                4,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, 0, 7 }  // master: level=70, transpose=0, feedback=7
        },
        // --- ROM1A #31: TRAIN ---
        {
            "TRAIN",
            4,             // algorithm_id (algo 5)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 1.0f, 64.0f, 0, false,
                    65, 24, 19, 57,  // rate
                    99, 85, 85, 0,  // level
                    3,               // rate_scaling
                    39, 0, 98, 3, 0,  // KLS: bp=39, ld=0, rd=98, lc=3, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 72, 3.0f, 1.0f, 0, false,
                    39, 13, 12, 72,  // rate
                    99, 61, 66, 0,  // level
                    5,               // rate_scaling
                    52, 0, 0, 3, 0,  // KLS: bp=52, ld=0, rd=0, lc=3, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 99, 22.0f, 57.0f, 2, true,
                    98, 29, 28, 33,  // rate
                    99, 0, 0, 0,  // level
                    0,               // rate_scaling
                    99, 98, 0, 1, 1,  // KLS: bp=99, ld=98, rd=0, lc=1, rc=1
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 89, 10.0f, 99.0f, -2, true,
                    98, 29, 28, 27,  // rate
                    99, 0, 0, 0,  // level
                    0,               // rate_scaling
                    20, 0, 0, 1, 1,  // KLS: bp=20, ld=0, rd=0, lc=1, rc=1
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 83, 9.0f, 0.0f, 3, false,
                    42, 17, 25, 53,  // rate
                    99, 99, 99, 99,  // level
                    0,               // rate_scaling
                    36, 0, 0, 3, 0,  // KLS: bp=36, ld=0, rd=0, lc=3, rc=0
                    0,               // velocity_sens
                    true,
                    3                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 99, 5.0f, 0.0f, 0, false,
                    49, 17, 25, 53,  // rate
                    99, 99, 99, 98,  // level
                    0,               // rate_scaling
                    36, 0, 0, 3, 0,  // KLS: bp=36, ld=0, rd=0, lc=3, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                0, 39, 0,   // wave, speed, delay
                0, 99,        // pm_depth, am_depth
                0,            // pitch_mod_sens
                false,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, 0, 7 }  // master: level=70, transpose=0, feedback=7
        },
        // --- ROM1A #32: TAKE OFF ---
        {
            "TAKE OFF",
            9,             // algorithm_id (algo 10)
            {{
                // Operator 1 (OP1)
                {
                    0, 99, 4.0f, 1.0f, 0, false,
                    9, 14, 17, 34,  // rate
                    61, 96, 0, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 2 (OP2)
                {
                    0, 96, 1.0f, 0.0f, 0, false,
                    82, 80, 19, 14,  // rate
                    80, 95, 0, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 3 (OP3)
                {
                    0, 99, 6.0f, 1.0f, 0, false,
                    76, 35, 99, 11,  // rate
                    67, 38, 73, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 4 (OP4)
                {
                    0, 99, 0.0f, 0.0f, 0, false,
                    13, 14, 20, 30,  // rate
                    99, 95, 99, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 5 (OP5)
                {
                    0, 96, 2.0f, 1.0f, 0, false,
                    88, 24, 23, 37,  // rate
                    99, 90, 0, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                },
                // Operator 6 (OP6)
                {
                    0, 99, 0.0f, 0.0f, 0, false,
                    89, 22, 20, 41,  // rate
                    99, 92, 0, 0,  // level
                    0,               // rate_scaling
                    0, 0, 0, 0, 0,  // KLS: bp=0, ld=0, rd=0, lc=0, rc=0
                    0,               // velocity_sens
                    true,
                    0                // amp_mod_sens
                }
            }},
            {},  // effects (default)
            {
                2, 65, 0,   // wave, speed, delay
                0, 0,        // pm_depth, am_depth
                5,            // pitch_mod_sens
                true,         // key_sync
                true          // osc_key_sync
            },  // lfo
            { 70, -24, 0 }  // master: level=70, transpose=-24, feedback=0
        }
    };
};

