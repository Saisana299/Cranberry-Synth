#include "modules/lfo.hpp"
#include "handlers/audio.hpp"
#include <algorithm>
#include <cmath>

// =============================================
// 波形名テーブル
// =============================================
static const char* const WAVE_NAMES[Lfo::WAVE_COUNT] = {
    "TRI", "SAW-", "SAW+", "SQR", "SIN", "S&H"
};

const char* Lfo::getWaveName(uint8_t w) {
    if (w >= WAVE_COUNT) return "???";
    return WAVE_NAMES[w];
}

// =============================================
// パラメータ → 内部値変換
// =============================================

/**
 * @brief Speed (0-99) → 1サンプルあたりの位相増分
 *
 * 近似: 周波数 ≈ 0.062 * 2^(speed * 0.0975)
 *   speed 0  → 約0.06 Hz
 *   speed 35 → 約1.9 Hz (デフォルト)
 *   speed 99 → 約50 Hz
 */
uint32_t Lfo::speedToPhaseDelta(uint8_t speed) {
    float freq = 0.062f * powf(2.0f, speed * 0.09753f);
    // delta = freq / SAMPLE_RATE * 2^32
    return static_cast<uint32_t>((freq / SAMPLE_RATE) * 4294967296.0);
}

/**
 * @brief Delay (0-99) → サンプル数
 *
 * ディレイ後にLFO効果がフェードインする時間。
 * 二次カーブで自然な応答:
 *   delay 0  → 0サンプル (即時)
 *   delay 50 → 約1.3秒
 *   delay 99 → 約5秒
 */
uint32_t Lfo::delayToSamples(uint8_t delay) {
    if (delay == 0) return 0;
    // 二次カーブ: (delay/99)^2 * 5秒
    float normalized = delay / 99.0f;
    return static_cast<uint32_t>(normalized * normalized * 5.0f * SAMPLE_RATE);
}

// =============================================
// 初期化・リセット
// =============================================

void Lfo::init() {
    reset();
}

void Lfo::reset() {
    phase_         = 0;
    delay_counter_ = 0;
    sh_value_      = 0;
    sh_prev_phase_ = 0;
    pitch_mod_out_ = 0;
    amp_mod_out_   = 0;
}

/**
 * @brief ノートオン時のLFOリセット
 *
 * key_sync有効時、位相を中間点(0x7FFFFFFF)にリセット。
 * これにより三角波はピークから開始し、サイン波はゼロクロスから開始する。
 */
void Lfo::keyOn() {
    if (key_sync_) {
        phase_         = 0x7FFFFFFFU;  // 中間点から開始
        delay_counter_ = 0;
        sh_value_      = 0;
        sh_prev_phase_ = 0;
    }
}

// =============================================
// メインループ処理
// =============================================

/**
 * @brief サンプル数分LFOを進行させる
 *
 * Synth::generate() から BUFFER_SIZE (128) サンプル単位で呼ばれる。
 * 位相を進め、ディレイランプを更新し、PM/AM出力を計算する。
 */
FASTRUN void Lfo::advance(uint32_t samples) {
    // S&H: ゼロクロス検出用に現在位相を保存
    Phase_t prev_phase = phase_;

    // 位相を進める
    phase_ += phase_delta_ * samples;

    // S&H: phaseが1周期のゼロクロスを跨いだら新しい値をホールド
    if (wave_ == Wave::SampleHold) {
        if (phase_ < prev_phase) {
            // オーバーフロー = 1周期完了 → 新しいランダム値
            // 簡易的な線形合同法 (LCG)
            sh_value_ = static_cast<int16_t>(
                ((sh_value_ * 1103515245 + 12345) >> 16) & 0x7FFF
            );
            // 符号付きに変換 [-Q15_MAX, Q15_MAX]
            sh_value_ = static_cast<int16_t>(sh_value_ - (Q15_MAX >> 1));
            sh_value_ = std::clamp<int16_t>(sh_value_, -Q15_MAX, Q15_MAX);
        }
        sh_prev_phase_ = phase_;
    }

    // ディレイカウンター更新
    if (delay_length_ > 0 && delay_counter_ < delay_length_) {
        delay_counter_ += samples;
        if (delay_counter_ > delay_length_) {
            delay_counter_ = delay_length_;
        }
    }

    // PM/AM出力を計算
    computeOutputs();
}

// =============================================
// 波形生成
// =============================================

/**
 * @brief 波形値を計算（位相アキュムレータから波形サンプルを生成）
 *
 * 全波形とも Wavetable (512サンプル, Q23形式) をルックアップし、
 * Q23 → Q15 変換 (>> 8) で [-Q15_MAX, Q15_MAX] の範囲で出力。
 *
 * @param phase 32bit位相 (0 ～ UINT32_MAX で1サイクル)
 * @return 波形サンプル値 [-Q15_MAX, Q15_MAX]
 */
int16_t Lfo::computeWaveform(Phase_t phase) const {
    // 共通: 32bit位相 → 512サンプルテーブルのインデックス変換
    // phase >> 23 = 32-9 → [0, 511], & 511 でラップ
    const uint32_t index = (phase >> 23) & 511;

    switch (wave_) {
        case Wave::Triangle:
            return static_cast<int16_t>(Wavetable::triangle[index] >> 8);

        case Wave::SawDown:
            // Wavetable::saw は +MAX→-MAX (下降) なのでそのまま
            return static_cast<int16_t>(Wavetable::saw[index] >> 8);

        case Wave::SawUp:
            // 符号反転で上昇ノコギリ波
            return static_cast<int16_t>(-(Wavetable::saw[index] >> 8));

        case Wave::Square:
            return static_cast<int16_t>(Wavetable::square[index] >> 8);

        case Wave::Sine:
            return static_cast<int16_t>(Wavetable::sine[index] >> 8);

        case Wave::SampleHold:
            return sh_value_;

        default:
            return 0;
    }
}

// =============================================
// ディレイ・出力計算
// =============================================

/**
 * @brief PM/AM出力を対数ドメインスケーリングで計算
 *
 * ディレイランプ:
 *   delay_counter_ / delay_length_ → [0, Q15_MAX] のリニアランプ
 *   delay_length_ == 0 の場合は即座に Q15_MAX
 *
 * PM出力 (対数周波数ドメイン):
 *   対数ドメイン計算:
 *     pmod_log = (pmd * pms * delay * lfo_centered) >> 22
 *   元式: (pmd*delay_Q24 * pms*(lfo-center)_Q24) >> 39 と等価
 *   （Q15→Q24のスケール差17ビットを補正: 39-17=22）
 *   pmod_logはlogfreq単位 (1<<24 = 1オクターブ)
 *   exp2f()で線形ドメインのQ15比率に変換
 *
 *   PMS=3, PMD=50 → 約±77セント
 *   PMS=7, PMD=99 → 約±1191セント
 *
 * AM出力:
 *   LFOの片側(正)をユニポーラ化 → [0, Q15_MAX]
 *   am_factor_ = am_depth * Q15_MAX / 99
 *   出力はSynthでオペレーター単位のAMS感度を掛けて使用
 */
void Lfo::computeOutputs() {
    int16_t raw = computeWaveform(phase_);

    // --- ディレイランプ [0, Q15_MAX] ---
    int32_t delay_ramp;
    if (delay_length_ == 0) {
        delay_ramp = Q15_MAX;
    } else if (delay_counter_ >= delay_length_) {
        delay_ramp = Q15_MAX;
    } else {
        delay_ramp = static_cast<int32_t>(
            (static_cast<int64_t>(delay_counter_) * Q15_MAX) / delay_length_
        );
    }

    // --- ピッチモジュレーション (対数ドメイン + exp2f変換) ---
    if (pmd_ != 0 && pms_ != 0) {
        // 対数ドメイン計算を Q15 スケールで再現:
        // 原式: pmod = (pmd * delay_Q24) * (pms * (lfo_val - (1<<23))) >> 39
        // Q15版: pmod = (pmd * delay_Q15) * (pms * raw_Q15) >> 22
        // （Q15→Q24変換の17ビット差: 39-17=22）
        int64_t depth_delay = static_cast<int64_t>(pmd_) * static_cast<int64_t>(delay_ramp);
        int64_t sens_wave   = static_cast<int64_t>(pms_) * static_cast<int64_t>(raw);
        int64_t pmod_log    = (depth_delay * sens_wave) >> 22;
        // pmod_log は logfreq 単位 (1<<24 = 1 octave)
        // 最大: (255 * 32767 * 255 * 32767) >> 22 ≈ 16,646,000 ≈ ±1191セント

        // 対数ドメインから線形ドメインへ変換:
        // freq_ratio = 2^(pmod_log / (1<<24)) - 1
        // これにより正負対称なピッチ変調を実現
        float log_octaves = static_cast<float>(pmod_log) * (1.0f / 16777216.0f);
        float ratio = exp2f(log_octaves) - 1.0f;
        pitch_mod_out_ = static_cast<int32_t>(ratio * 32767.0f);

        // クランプ（位相デルタが0以下にならないよう保護）
        if (pitch_mod_out_ > Q15_MAX) pitch_mod_out_ = Q15_MAX;
        if (pitch_mod_out_ < -30000) pitch_mod_out_ = -30000;  // delta * 0.08 が下限
    } else {
        pitch_mod_out_ = 0;
    }

    // --- アンプモジュレーション ---
    if (am_factor_ != 0) {
        // raw [-Q15_MAX, Q15_MAX] → [0, Q15_MAX] (片側: 音量を下げる方向のみ)
        int32_t unipolar = (static_cast<int32_t>(raw) + Q15_MAX) >> 1;
        // ディレイ適用
        int32_t ramped = (unipolar * delay_ramp) >> Q15_SHIFT;
        // am_factor_ 適用
        int32_t am = (ramped * am_factor_) >> Q15_SHIFT;
        amp_mod_out_ = static_cast<Gain_t>(std::clamp<int32_t>(am, 0, Q15_MAX));
    } else {
        amp_mod_out_ = 0;
    }
}

// =============================================
// スケールファクター計算
// =============================================

/**
 * @brief PM/AMスケールファクターを再計算
 *
 * PM:
 *   pmd_ = (pm_depth * 165) >> 6  (0-255)
 *   pms_ = PMS_TAB[pitch_mod_sens_] (0-255)
 *   computeOutputs() で対数ドメイン計算 + exp2f() 線形変換を行う
 *
 * AM:
 *   am_factor_ = am_depth * Q15_MAX / 99
 *   出力 [0, Q15_MAX] をSynthでオペレーター単位AMS感度と乗算
 */
void Lfo::updateScaleFactors() {
    // PM: パラメータをキャッシュ
    pmd_ = (pm_depth_ > 0) ?
        static_cast<uint8_t>((static_cast<uint16_t>(pm_depth_) * 165) >> 6) : 0;
    pms_ = (pitch_mod_sens_ > 0) ? PMS_TAB[pitch_mod_sens_] : 0;

    // AM ファクター (Synthでオペレーター単位AMS感度を適用)
    if (am_depth_ > 0) {
        am_factor_ = static_cast<int32_t>(
            (static_cast<int64_t>(am_depth_) * Q15_MAX) / 99
        );
    } else {
        am_factor_ = 0;
    }
}

// =============================================
// パラメータセッター
// =============================================

void Lfo::setWave(uint8_t w) {
    if (w >= WAVE_COUNT) w = 0;
    wave_ = static_cast<Wave>(w);
}

void Lfo::setSpeed(uint8_t s) {
    speed_ = std::min<uint8_t>(s, 99);
    phase_delta_ = speedToPhaseDelta(speed_);
}

void Lfo::setDelay(uint8_t d) {
    delay_param_ = std::min<uint8_t>(d, 99);
    delay_length_ = delayToSamples(delay_param_);
}

void Lfo::setPmDepth(uint8_t d) {
    pm_depth_ = std::min<uint8_t>(d, 99);
    updateScaleFactors();
}

void Lfo::setAmDepth(uint8_t d) {
    am_depth_ = std::min<uint8_t>(d, 99);
    updateScaleFactors();
}

void Lfo::setPitchModSens(uint8_t s) {
    pitch_mod_sens_ = std::min<uint8_t>(s, 7);
    updateScaleFactors();
}

void Lfo::setKeySync(bool sync) {
    key_sync_ = sync;
}
