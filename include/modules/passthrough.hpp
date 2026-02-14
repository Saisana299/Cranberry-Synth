#pragma once

#include "handlers/audio.hpp"
#include "modules/filter.hpp"
#include "modules/delay.hpp"

/**
 * @brief PCM1802 パススルーモジュール
 *
 * I2S 入力 (PCM1802 ADC) の音声をそのまま I2SQuad DAC へ出力する。
 * パススルーモード中はシンセ機能を停止し、入力→出力の低遅延パスを実現する。
 * LPF, HPF, Delay エフェクトを適用可能。
 */
class Passthrough {
public:
    explicit Passthrough(AudioHandler& audio) : audio_(audio) {}

    /** @brief パススルーモード開始 (RecordQueue を有効化) */
    void begin();

    /** @brief パススルーモード終了 (RecordQueue を停止) */
    void end();

    /** @brief 毎ループ呼び出し: 入力を読み取り差動出力へ書き出す */
    void process();

    bool isActive() const { return active_; }

    // --- エフェクト制御 ---
    void setLpfEnabled(bool enabled) { lpf_enabled_ = enabled; }
    void setHpfEnabled(bool enabled) { hpf_enabled_ = enabled; }
    void setDelayEnabled(bool enabled) { delay_enabled_ = enabled; }
    bool isLpfEnabled() const { return lpf_enabled_; }
    bool isHpfEnabled() const { return hpf_enabled_; }
    bool isDelayEnabled() const { return delay_enabled_; }

    Filter& getFilter() { return filter_; }
    Delay&  getDelay()  { return delay_; }

private:
    AudioHandler& audio_;
    bool active_ = false;

    // エフェクト
    Filter filter_;
    Delay  delay_;
    bool lpf_enabled_   = false;
    bool hpf_enabled_   = false;
    bool delay_enabled_ = false;

    /** @brief 符号反転 (INT16_MIN のオーバーフロー対策付き) */
    static inline Sample16_t negation(Sample16_t value) {
        return (value == INT16_MIN) ? INT16_MAX : static_cast<Sample16_t>(-value);
    }
};
