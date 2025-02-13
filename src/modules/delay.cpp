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
    // todo 条件式

    this->time = time;
    this->level = level;
    this->feedback = feedback;

    this->delay_length = getTotalSamples();

    uint32_t delay_sample = static_cast<uint32_t>((this->time * SAMPLE_RATE) >> 10);
    buffer_L.setInterval(delay_sample);
    buffer_R.setInterval(delay_sample);
}

/**
 * @brief ディレイ処理
 *
 * @param in サンプル
 * @param is_r 右チャンネルか
 * @return int16_t 処理後のサンプル
 */
int16_t Delay::process(int16_t in, bool is_r) {
    IntervalRingBuffer<int16_t, 13230>& buf = (is_r ? buffer_R : buffer_L);

    int32_t out_temp;
    int32_t fb_temp;
    int32_t sample = static_cast<int32_t>(buf.read());

    // 出力用計算
    out_temp = in + ((level * sample) >> 10);
    out_temp = std::clamp<int32_t>(out_temp, -32768, 32767);

    // フィードバック用計算
    fb_temp = in + ((feedback * sample) >> 10);
    fb_temp = std::clamp<int32_t>(fb_temp, -32768, 32767);

    buf.write(static_cast<int16_t>(fb_temp));
    buf.update();

    return static_cast<int16_t>(out_temp);
}

/** @brief ディレイが続く時間を計算 */
uint32_t Delay::getTotalSamples() {
    // 1024(1.0) = Unlimited
    float feedback_ratio = static_cast<float>(feedback) / 1024.0f;
    if (feedback_ratio >= 1.0f) return UINT32_MAX;

    // 60dB減衰するまでのエコー回数を計算
    float n = log(0.001f) / log(feedback_ratio);

    // 全体の残響時間をミリ秒で計算
    float total_time = n * static_cast<float>(time);

    // 残響時間をサンプル数に変換
    uint32_t total_samples = (static_cast<uint32_t>(total_time * static_cast<float>(SAMPLE_RATE))) >> 10;

    return total_samples;
}

/** @brief ディレイが続く時間を取得 */
uint32_t Delay::getDelayLength() {
    return delay_length;
}