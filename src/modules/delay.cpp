#include "modules/delay.h"

void Delay::reset() {
    delay_length = 0;
    buffer_L.reset();
    buffer_R.reset();
}

void Delay::setDelay(float time, float level, float feedback) {
    // todo 条件式

    this->time = time;
    this->level = level;
    this->feedback = feedback;

    this->delay_length = getTotalSamples();

    uint32_t delay_sample = SAMPLE_RATE * this->time;
    buffer_L.setInterval(delay_sample);
    buffer_R.setInterval(delay_sample);
}

int16_t Delay::process(int16_t in, bool isR) {
    int16_t temp;

    // Left
    if(!isR) {
        temp = in + (int16_t)(level * buffer_L.read());
        buffer_L.write(in + (int16_t)(feedback * buffer_L.read()));
        buffer_L.update();
    }
    // Right
    else {
        temp = in + (int16_t)(level * buffer_R.read());
        buffer_R.write(in + (int16_t)(feedback * buffer_R.read()));
        buffer_R.update();
    }
    return temp;
}

uint32_t Delay::getTotalSamples() {
    // 1.0 = Unlimited
    if (feedback >= 1.0f) return UINT32_MAX;

    // 60dB減衰するまでのエコー回数を計算
    float n = log(0.001f) / log(feedback);

    // 全体の残響時間をミリ秒で計算
    float total_time = n * time;

    // 残響時間をサンプル数に変換
    uint32_t total_samples = (uint32_t)(total_time * SAMPLE_RATE);

    return total_samples;
}

uint32_t Delay::getDelayLength() {
    return delay_length;
}