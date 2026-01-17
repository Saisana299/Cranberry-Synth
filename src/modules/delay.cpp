#include "modules/delay.hpp"

/** @brief ディレイのバッファをリセット */
void Delay::reset() {
    delay_length = 0;
    buffer_L.reset();
    buffer_R.reset();
}

/**
 * @brief ディレイの設定
 *
 * @param time ディレイ時間(ms)
 * @param level 出力レベル (Q15: 0-32767)
 * @param feedback フィードバック (Q15: 0-32767)
 */
void Delay::setDelay(int32_t time, Gain_t level, Gain_t feedback) {
    setTime(time);
    setLevel(level);
    setFeedback(feedback);
}

void Delay::setTime(int32_t time) {
    this->time = std::clamp<int32_t>(time, MIN_TIME, MAX_TIME);
    this->delay_length = getTotalSamples();

    uint32_t delay_sample = (static_cast<uint32_t>(this->time) * SAMPLE_RATE) / 1000;
    buffer_L.setInterval(delay_sample);
    buffer_R.setInterval(delay_sample);
}

void Delay::setLevel(Gain_t level) {
    this->level = clamp_gain(level, MIN_LEVEL, MAX_LEVEL);
}

void Delay::setFeedback(Gain_t feedback) {
    this->feedback = clamp_gain(feedback, MIN_FEEDBACK, MAX_FEEDBACK);
    this->delay_length = getTotalSamples();
}

/**
 * @brief ディレイ処理
 *
 * @param in サンプル
 * @return Sample16_t 処理後のサンプル
 */
Sample16_t Delay::processL(Sample16_t in) {
    int32_t sample = static_cast<int32_t>(buffer_L.read());

    // Q15乗算: sample × level >> 15
    int32_t out_temp = in + ((level * sample) >> Q15_SHIFT);
    out_temp = std::clamp<int32_t>(out_temp, SAMPLE16_MIN, SAMPLE16_MAX);

    int32_t fb_temp = in + ((feedback * sample) >> Q15_SHIFT);
    fb_temp = std::clamp<int32_t>(fb_temp, SAMPLE16_MIN, SAMPLE16_MAX);

    buffer_L.write(static_cast<Sample16_t>(fb_temp));
    buffer_L.update();

    return static_cast<Sample16_t>(out_temp);
}

/**
 * @brief ディレイ処理
 *
 * @param in サンプル
 * @return Sample16_t 処理後のサンプル
 */
Sample16_t Delay::processR(Sample16_t in) {
    int32_t sample = static_cast<int32_t>(buffer_R.read());

    // Q15乗算: sample × level >> 15
    int32_t out_temp = in + ((level * sample) >> Q15_SHIFT);
    out_temp = std::clamp<int32_t>(out_temp, SAMPLE16_MIN, SAMPLE16_MAX);

    int32_t fb_temp = in + ((feedback * sample) >> Q15_SHIFT);
    fb_temp = std::clamp<int32_t>(fb_temp, SAMPLE16_MIN, SAMPLE16_MAX);

    buffer_R.write(static_cast<Sample16_t>(fb_temp));
    buffer_R.update();

    return static_cast<Sample16_t>(out_temp);
}

/** @brief ディレイが続く時間を計算 */
uint32_t Delay::getTotalSamples() const {
    float feedback_ratio = getFeedbackRatio();

    if (feedback_ratio >= 1.0f) return UINT32_MAX;
    if (feedback_ratio <= 0.001f) return 0;

    // 60dB減衰するまでのエコー回数を計算
    float n = log(0.001f) / log(feedback_ratio);

    // 全体の残響時間をミリ秒で計算
    float total_time_ms = n * static_cast<float>(time);

    // 残響時間をサンプル数に変換
    uint32_t total_samples = static_cast<uint32_t>(total_time_ms * SAMPLE_RATE / 1000.0f);

    return total_samples;
}

/** @brief ディレイが続く時間を取得 */
uint32_t Delay::getDelayLength() const {
    return delay_length;
}