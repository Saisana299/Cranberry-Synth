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
 * @param level 出力レベル(100% = 1024)
 * @param feedback フィードバック(100% = 1024)
 */
void Delay::setDelay(int32_t time, int32_t level, int32_t feedback) {
    // 検証
    this->time = clamp_param(time, MIN_TIME, MAX_TIME);
    this->level = clamp_param(level, MIN_LEVEL, MAX_LEVEL);
    this->feedback = clamp_param(feedback, MIN_FEEDBACK, MAX_FEEDBACK);

    this->delay_length = getTotalSamples();

    uint32_t delay_sample = (static_cast<uint32_t>(this->time * SAMPLE_RATE) >> FIXED_POINT_SHIFT);
    buffer_L.setInterval(delay_sample);
    buffer_R.setInterval(delay_sample);
}

/**
 * @brief ディレイ処理
 *
 * @param in サンプル
 * @return int16_t 処理後のサンプル
 */
int16_t Delay::processL(int16_t in) {
    int32_t sample = static_cast<int32_t>(buffer_L.read());

    int32_t out_temp = in + ((level * sample) >> FIXED_POINT_SHIFT);
    out_temp = std::clamp<int32_t>(out_temp, -32768, 32767);

    int32_t fb_temp = in + ((feedback * sample) >> FIXED_POINT_SHIFT);
    fb_temp = std::clamp<int32_t>(fb_temp, -32768, 32767);

    buffer_L.write(static_cast<int16_t>(fb_temp));
    buffer_L.update();

    return static_cast<int16_t>(out_temp);
}

/**
 * @brief ディレイ処理
 *
 * @param in サンプル
 * @return int16_t 処理後のサンプル
 */
int16_t Delay::processR(int16_t in) {
    int32_t sample = static_cast<int32_t>(buffer_R.read());

    int32_t out_temp = in + ((level * sample) >> FIXED_POINT_SHIFT);
    out_temp = std::clamp<int32_t>(out_temp, -32768, 32767);

    int32_t fb_temp = in + ((feedback * sample) >> FIXED_POINT_SHIFT);
    fb_temp = std::clamp<int32_t>(fb_temp, -32768, 32767);

    buffer_R.write(static_cast<int16_t>(fb_temp));
    buffer_R.update();

    return static_cast<int16_t>(out_temp);
}

/** @brief ディレイが続く時間を計算 */
uint32_t Delay::getTotalSamples() const {
    float feedback_ratio = getFeedbackRatio();

    if (feedback_ratio >= 1.0f) return UINT32_MAX;

    // 60dB減衰するまでのエコー回数を計算
    float n = log(0.001f) / log(feedback_ratio);

    // 全体の残響時間をミリ秒で計算
    float total_time = n * static_cast<float>(time);

    // 残響時間をサンプル数に変換
    uint32_t total_samples = (static_cast<uint32_t>(total_time * static_cast<float>(SAMPLE_RATE))) >> FIXED_POINT_SHIFT;

    return total_samples;
}

/** @brief ディレイが続く時間を取得 */
uint32_t Delay::getDelayLength() const {
    return delay_length;
}