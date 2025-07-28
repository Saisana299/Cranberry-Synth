#pragma once

#include "handlers/audio.hpp"
#include "utils/math.hpp"

class Envelope {

private:
    static const uint32_t RATE_TABLE_SIZE = 100;
    static const uint32_t EXP_TABLE_SIZE = 4096;

    // 固定小数点のシフト量を定義 (8ビット = 1/256)
    static constexpr int FIXED_POINT_SHIFT = 8;

    // 非線形レートと対数サステインレベルを保持する変数
    // レートの最大値は (EXP_TABLE_SIZE - 1) << FIXED_POINT_SHIFT まで
    uint32_t attack_rate = (EXP_TABLE_SIZE - 1) << FIXED_POINT_SHIFT;
    uint32_t decay_rate = 4095;
    uint32_t release_rate = 20;
    uint32_t sustain_log_level = 0;

    // rate_tableを再設計
    // 音楽的な時間(ms)からレート値を計算するヘルパー
    static constexpr uint32_t ms_to_rate(double ms) {
        if (ms <= 0) return (EXP_TABLE_SIZE << FIXED_POINT_SHIFT);
        // (4095スケール * 固定小数点スケール) / (時間ms * 1秒あたりのサンプル数)
        return static_cast<uint32_t>(((EXP_TABLE_SIZE -1) << FIXED_POINT_SHIFT) / (ms / 1000.0 * SAMPLE_RATE));
    }

    static constexpr std::array<uint32_t, RATE_TABLE_SIZE> generate_rate_table() {
        std::array<uint32_t, RATE_TABLE_SIZE> table{};
        // パラメータ0-99を、10秒(10000ms)からほぼ0秒までの対数的なカーブにマッピング
        for (size_t i = 0; i < RATE_TABLE_SIZE; ++i) {
            double time_ms = 10000.0 * std::pow(0.9, i * 0.7);
            table[99 - i] = ms_to_rate(time_ms);
        }
        table[99] = ms_to_rate(0); // レート99は最速
        return table;
    }

    // 配列のサイズは EXP_TABLE_SIZE に合わせる
    static constexpr std::array<int16_t, EXP_TABLE_SIZE> generate_exp_table() {
        std::array<int16_t, EXP_TABLE_SIZE> table{};
        for(size_t i = 0; i < EXP_TABLE_SIZE; ++i) {
            table[i] = static_cast<int16_t>(1023.0 * std::exp(-static_cast<double>(i) / 512.0));
        }
        return table;
    }

public:

    // レートテーブル（0-99）-> 実際の増分値へ。非線形なカーブを持つ。
    inline static const std::array<uint32_t, RATE_TABLE_SIZE> rate_table = generate_rate_table();

    // 指数テーブル（対数レベル -> 線形レベルへ）
    // インデックス = 0 で最大レベル、EXP_TABLE_SIZE - 1 で最小レベル
    inline static const std::array<int16_t, EXP_TABLE_SIZE> exp_table = generate_exp_table();

    enum class State {
        Attack, Decay, Sustain, Release
    };

    struct Memory {
        State state = State::Attack;
        uint32_t log_level = (EXP_TABLE_SIZE - 1) << FIXED_POINT_SHIFT; // 対数スケールの内部レベル（減衰量）
        int16_t current_level = 0;  // 線形スケールの最終出力レベル
    };

    void reset(Memory& mem);
    void release(Memory& mem);
    void update(Memory& mem);

    void setAttack(uint8_t attack_rate_0_99);
    void setDecay(uint8_t decay_rate_0_99);
    void setRelease(uint8_t release_rate_0_99);
    void setSustain(int16_t sustain_level_0_1023);

    /**
     * @brief 現在のレベルを返します
     *
     * @return int16_t Min: 0, Max: 1024
     */
    inline int16_t currentLevel(Memory& mem) {
        return mem.current_level;
    }

    /**
     * @brief エンベロープ終了判定
     *
     * @return 終了していれば `true` を返す
     */
    inline bool isFinished(Memory& mem) {
        const uint32_t max_attenuation = (EXP_TABLE_SIZE - 1) << FIXED_POINT_SHIFT;
        return (mem.state == State::Release && mem.log_level >= max_attenuation);
    }
};