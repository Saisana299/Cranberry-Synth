#pragma once

#include "handlers/audio.hpp"
#include "utils/math.hpp"
#include "types.hpp"

/**
 * @brief FM方式エンベロープジェネレーター
 *
 * 4つのRate (R1-R4) と 4つのLevel (L1-L4) で構成される。
 *
 * 動作フロー:
 * - Phase1: 現在レベル → L1 (Rate1で移動) [アタック]
 * - Phase2: L1 → L2 (Rate2で移動) [ディケイ1]
 * - Phase3: L2 → L3 (Rate3で移動) [ディケイ2/サステイン]
 * - Phase4: L3 → L4 (Rate4で移動) [リリース] ※ノートオフで発動
 *
 * Rate: 0=最も遅い, 99=最も速い（即座に到達）
 * Level: 0=無音, 99=最大音量
 */
class Envelope {

public:
    static constexpr uint32_t RATE_TABLE_SIZE = 100;
    static constexpr uint32_t LEVEL_TABLE_SIZE = 100;
    static constexpr uint32_t EXP_TABLE_SIZE = 4096;
    static constexpr uint32_t EXP_TABLE_MASK = EXP_TABLE_SIZE - 1; // 0xFFF

    // 固定小数点のシフト量を定義 (8ビット = 1/256)
    static constexpr int FIXED_POINT_SHIFT = 8;

    static constexpr uint32_t MAX_ATTENUATION = (EXP_TABLE_SIZE - 1) << FIXED_POINT_SHIFT;

    // エンベロープステート
    enum class EnvelopeState {
        Phase1,  // → L1 (アタック)
        Phase2,  // L1 → L2 (ディケイ1)
        Phase3,  // L2 → L3 (ディケイ2/サステイン)
        Phase4,  // → L4 (リリース)
        Idle     // 完了/待機
    };

    struct Memory {
        EnvelopeState state = EnvelopeState::Idle;
        uint32_t log_level = MAX_ATTENUATION; // 対数スケールの内部レベル（減衰量）
        Gain_t current_level = 0;  // 線形スケールの最終出力レベル (Q15: 0-32767)
        int8_t rate_scaling_delta = 0; // Rate Scalingによるrate増分 (ノートごとに計算)
    };

private:
    // Rate/Level形式のパラメータ (0-99)
    // Rate: 変化速度 (0=遅い, 99=即座)
    // Level: 目標レベル (0=無音, 99=最大)
    uint8_t rate1_param = 99;   // アタックレート
    uint8_t rate2_param = 99;   // ディケイ1レート
    uint8_t rate3_param = 99;   // ディケイ2レート
    uint8_t rate4_param = 99;   // リリースレート
    uint8_t level1_param = 99;  // アタック到達レベル
    uint8_t level2_param = 99;  // ディケイ1到達レベル
    uint8_t level3_param = 99;  // サステインレベル
    uint8_t level4_param = 0;   // リリース到達レベル（通常0）

    uint8_t rate_scaling_param = 0; // Rate Scaling sensitivity (0-7)

    // 内部用変換済み値
    uint32_t rate1 = MAX_ATTENUATION;
    uint32_t rate2 = MAX_ATTENUATION;
    uint32_t rate3 = MAX_ATTENUATION;
    uint32_t rate4 = MAX_ATTENUATION;
    uint32_t target_level1 = 0;             // L1の減衰量
    uint32_t target_level2 = 0;             // L2の減衰量
    uint32_t target_level3 = 0;             // L3の減衰量
    uint32_t target_level4 = MAX_ATTENUATION; // L4の減衰量

    /**
     * @brief レートテーブルを生成
     *
     * Rate 0 = 約28秒, Rate 13 = 約16秒, Rate 30 = 約8秒, Rate 99 = 約2ms
     * クリックノイズ防止のため、Rate 99でも最低2msのスムージング時間を確保
     */
    static constexpr std::array<uint32_t, RATE_TABLE_SIZE> generate_rate_table() {
        std::array<uint32_t, RATE_TABLE_SIZE> table{};

        // Rate 13: 約16秒, Rate 30: 約8秒, Rate 50: 約3秒
        // Rate 70: 約1.3秒, Rate 90: 約0.6秒, Rate 99: 約2ms（クリックノイズ防止）

        // 最小時間: 2ms（クリックノイズ防止、業界標準）
        constexpr double MIN_TIME_SECONDS = 0.002;

        for (size_t i = 0; i < RATE_TABLE_SIZE; ++i) {
            double time_seconds = 0.0;

            // 指数カーブ: time = 28 * 2^(-rate/16)
            // 約16レートごとに半減（緩やかなカーブ）
            double rate_normalized = static_cast<double>(i) / 16.0;
            time_seconds = 28.0 * std::pow(0.5, rate_normalized);

            // 最小値を5msに制限（クリックノイズ防止）
            if (time_seconds < MIN_TIME_SECONDS) time_seconds = MIN_TIME_SECONDS;

            // time_seconds で MAX_ATTENUATION を移動するための1サンプルあたりの増分
            double samples = time_seconds * SAMPLE_RATE;
            if (samples < 1.0) samples = 1.0;

            table[i] = static_cast<uint32_t>(MAX_ATTENUATION / samples);
            if (table[i] == 0) table[i] = 1; // 最低でも1
        }
        return table;
    }

    // 配列のサイズは EXP_TABLE_SIZE に合わせる
    // 指数テーブル: 対数レベル(減衰量) → 線形レベル(音量)
    // インデックス0で最大音量(Q15_MAX)、最大インデックスで無音(0)
    static constexpr std::array<Gain_t, EXP_TABLE_SIZE> generate_exp_table() {
        std::array<Gain_t, EXP_TABLE_SIZE> table{};
        // ダイナミックレンジ: 96dB
        // これにより減衰がより自然になる
        constexpr double TOTAL_DB_RANGE = 96.0;
        for(size_t i = 0; i < EXP_TABLE_SIZE; ++i) {
            double normalized = static_cast<double>(i) / (EXP_TABLE_SIZE - 1);
            // dB = -96 * normalized
            double db = -TOTAL_DB_RANGE * normalized;
            // dB → 線形スケール: 10^(dB/20)
            double linear = std::pow(10.0, db / 20.0);
            table[i] = static_cast<Gain_t>(Q15_MAX * linear);
        }
        return table;
    }

    /**
     * @brief レベル->減衰量 変換テーブルを生成
     *
     * レベル0=無音（最大減衰）、レベル99=最大音量（減衰なし）
     */
    static constexpr std::array<uint32_t, LEVEL_TABLE_SIZE> generate_level_to_attenuation_table() {
        std::array<uint32_t, LEVEL_TABLE_SIZE> table{};

        // Level 0-99 を TL (Total Level) 0-127 相当に変換
        // TL 0 = 最大音量, TL 127 = 無音
        // AudioMath::LEVEL_TO_TL と同じ値
        constexpr uint8_t level_lut[100] = {
            127, 122, 118, 114, 110, 107, 104, 102, 100, 98,  // 0-9
            96, 94, 92, 90, 88, 86, 85, 84, 82, 81,           // 10-19
            79, 78, 77, 76, 75, 74, 73, 72, 71, 70,           // 20-29
            69, 68, 67, 66, 65, 64, 63, 62, 61, 60,           // 30-39
            59, 58, 57, 56, 55, 54, 53, 52, 51, 50,           // 40-49
            49, 48, 47, 46, 45, 44, 43, 42, 41, 40,           // 50-59
            39, 38, 37, 36, 35, 34, 33, 32, 31, 30,           // 60-69
            29, 28, 27, 26, 25, 24, 23, 22, 21, 20,           // 70-79
            19, 18, 17, 16, 15, 14, 13, 12, 11, 10,           // 80-89
            9, 8, 7, 6, 5, 4, 3, 2, 1, 0                      // 90-99
        };

        for (size_t i = 0; i < LEVEL_TABLE_SIZE; ++i) {
            // 減衰量にスケーリング
            // TL 0 → 減衰量 0 (最大音量)
            // TL 127 → 減衰量 MAX_ATTENUATION (無音)
            uint32_t tl = level_lut[i];
            table[i] = (tl * MAX_ATTENUATION) / 127;
        }
        return table;
    }

    static inline uint8_t clamp_param(uint8_t value) {
        return (value >= RATE_TABLE_SIZE) ? (RATE_TABLE_SIZE - 1) : value;
    }

    // レートテーブル（0-99）-> 実際の増分値へ。非線形なカーブを持つ。
    inline static const std::array<uint32_t, RATE_TABLE_SIZE> rate_table = generate_rate_table();

    // 指数テーブル（対数レベル -> 線形レベルへ）
    // インデックス = 0 で最大レベル(Q15_MAX)、EXP_TABLE_SIZE - 1 で最小レベル(0)
    inline static const std::array<Gain_t, EXP_TABLE_SIZE> exp_table = generate_exp_table();

    // レベル -> 減衰量 変換テーブル
    inline static const std::array<uint32_t, LEVEL_TABLE_SIZE> level_to_attenuation_table = generate_level_to_attenuation_table();

public:
    void reset(Memory& mem);
    void release(Memory& mem);
    void clear(Memory& mem);  // 完全にIdle状態にリセット

    FASTRUN void update(Memory& mem);

    // Rate設定 (0-99: 0=遅い, 99=即座)
    void setRate1(uint8_t rate_0_99);
    void setRate2(uint8_t rate_0_99);
    void setRate3(uint8_t rate_0_99);
    void setRate4(uint8_t rate_0_99);

    // Level設定 (0-99: 0=無音, 99=最大)
    void setLevel1(uint8_t level_0_99);
    void setLevel2(uint8_t level_0_99);
    void setLevel3(uint8_t level_0_99);
    void setLevel4(uint8_t level_0_99);

    /**
     * @brief 現在のレベルを返す
     *
     * @return Gain_t Q15スケール (0-32767)
     */
    inline Gain_t currentLevel(const Memory& mem) const {
        return mem.current_level;
    }

    /**
     * @brief エンベロープ終了判定
     *
     * Idle状態、または音量レベルが十分小さい場合（聞こえない程度）に終了とみなす
     *
     * @return 終了していれば `true` を返す
     */
    inline bool isFinished(const Memory& mem) const {
        // Idle状態なら終了
        if (mem.state == EnvelopeState::Idle) return true;
        // Phase4（リリース中）で音量が非常に小さい場合も終了とみなす
        // current_level は Q15スケール (0-32767)、64未満なら実質無音
        if (mem.state == EnvelopeState::Phase4 && mem.current_level < 64) return true;
        return false;
    }

    // Rate ゲッター (0-99)
    inline uint8_t getRate1() const { return rate1_param; }
    inline uint8_t getRate2() const { return rate2_param; }
    inline uint8_t getRate3() const { return rate3_param; }
    inline uint8_t getRate4() const { return rate4_param; }

    // Level ゲッター (0-99)
    inline uint8_t getLevel1() const { return level1_param; }
    inline uint8_t getLevel2() const { return level2_param; }
    inline uint8_t getLevel3() const { return level3_param; }
    inline uint8_t getLevel4() const { return level4_param; }

    // Rate Scaling 設定/取得 (0-7)
    void setRateScaling(uint8_t sensitivity);
    inline uint8_t getRateScaling() const { return rate_scaling_param; }

    /**
     * @brief Rate Scaling計算
     *
     * MIDIノート番号とsensitivityからrate増分を計算
     * 高いノートほどエンベロープが速くなる
     *
     * @param midinote MIDIノート番号 (0-127)
     * @param sensitivity Rate Scaling sensitivity (0-7)
     * @return int8_t rate増分 (0-27)
     */
    static int8_t calcRateScalingDelta(uint8_t midinote, uint8_t sensitivity);

    /**
     * @brief ノートごとのRate Scaling増分を設定
     *
     * @param mem エンベロープメモリ
     * @param midinote MIDIノート番号
     */
    void applyRateScaling(Memory& mem, uint8_t midinote);
};
