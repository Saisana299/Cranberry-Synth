#pragma once

#include "types.hpp"
#include "utils/wavetable.hpp"

/**
 * @brief LFOモジュール
 *
 * 波形生成・ディレイランプ・PM/AM出力を担当。
 * Synth::generate() の各バッファ処理前に advance() で位相を進め、
 * getPitchMod() / getAmpMod() で変調値を取得する。
 *
 * 出力仕様:
 *   getPitchMod() → int32_t: ピッチ変調量（符号付きQ15比率）
 *     phase_offset = (delta * getPitchMod()) >> Q15_SHIFT
 *     対数周波数ドメイン計算をexp2f()で線形変換
 *     PMS=3,PMD=50で約±77セント
 *     PMS=7,PMD=99で約±1191セント
 *   getAmpMod()   → Gain_t:  振幅変調量 [0, Q15_MAX]
 *     オペレーター単位でAMS感度を掛けて使用
 */
class Lfo {
public:
    // 6波形
    enum class Wave : uint8_t {
        Triangle   = 0,
        SawDown    = 1,
        SawUp      = 2,
        Square     = 3,
        Sine       = 4,
        SampleHold = 5
    };

    static constexpr uint8_t WAVE_COUNT = 6;

    // PMS感度テーブル (pitchmodsenstab)
    static constexpr uint8_t PMS_TAB[8] = {0, 10, 20, 33, 55, 92, 153, 255};

    // AMS感度テーブル (Q15スケール)
    // 原典 Q24: {0, 4342338, 7171437, 16777216} → Q15: {0, 8484, 14013, 32767}
    static constexpr Gain_t AMS_TAB[4] = {0, 8484, 14013, Q15_MAX};

    void init();
    void reset();                           // プリセットロード時のリセット
    void keyOn();                           // ノートオン時 (key_sync考慮)
    void advance(uint32_t samples);         // サンプル数分フェーズを進める (FASTRUN in .cpp)

    // --- 出力取得 ---
    int32_t getPitchMod() const { return pitch_mod_out_; }
    Gain_t  getAmpMod()   const { return amp_mod_out_; }

    // --- パラメータ設定 ---
    void setWave(uint8_t w);
    void setSpeed(uint8_t s);
    void setDelay(uint8_t d);
    void setPmDepth(uint8_t d);
    void setAmDepth(uint8_t d);
    void setPitchModSens(uint8_t s);
    void setKeySync(bool sync);

    // --- パラメータ取得 (UI用) ---
    uint8_t getWave()         const { return static_cast<uint8_t>(wave_); }
    uint8_t getSpeed()        const { return speed_; }
    uint8_t getDelay()        const { return delay_param_; }
    uint8_t getPmDepth()      const { return pm_depth_; }
    uint8_t getAmDepth()      const { return am_depth_; }
    uint8_t getPitchModSens() const { return pitch_mod_sens_; }
    bool    getKeySync()      const { return key_sync_; }

    // --- ユーティリティ ---
    static const char* getWaveName(uint8_t w);

private:
    // パラメータ
    Wave    wave_           = Wave::Triangle;
    uint8_t speed_          = 35;    // 0-99
    uint8_t delay_param_    = 0;     // 0-99
    uint8_t pm_depth_       = 0;     // 0-99
    uint8_t am_depth_       = 0;     // 0-99
    uint8_t pitch_mod_sens_ = 3;     // 0-7
    bool    key_sync_       = true;

    // 内部状態
    Phase_t  phase_         = 0;     // 位相アキュムレータ
    uint32_t phase_delta_   = 0;     // 1サンプルあたりの位相増分
    uint32_t delay_counter_ = 0;     // ディレイ経過サンプル数
    uint32_t delay_length_  = 0;     // ディレイ長（サンプル数）
    int16_t  sh_value_      = 0;     // S&H ホールド値
    Phase_t  sh_prev_phase_ = 0;     // S&H ゼロクロス検出用

    // プリコンピュート済みスケールファクター
    uint8_t pmd_ = 0;           // (pm_depth * 165) >> 6, range 0-255
    uint8_t pms_ = 0;           // PMS_TAB[pitch_mod_sens_], range 0-255
    int32_t am_factor_ = 0;     // am_depth のスケール

    // 出力キャッシュ（advance() で計算、getter で取得）
    int32_t pitch_mod_out_ = 0;
    Gain_t  amp_mod_out_   = 0;

    // 内部メソッド
    int16_t  computeWaveform(Phase_t phase) const;  // [-Q15_MAX, Q15_MAX]
    void     computeOutputs();                       // PM/AM出力を計算
    void     updateScaleFactors();                   // スケールファクター再計算

    // パラメータ → 内部値変換
    static uint32_t speedToPhaseDelta(uint8_t speed);
    static uint32_t delayToSamples(uint8_t delay);
};
