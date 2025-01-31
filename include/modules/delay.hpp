#pragma once

#include "handlers/audio.hpp"
#include "utils/buffer.hpp"

class Delay {
private:
    // 13230samples = 44100hz * 0.3s
    IntervalRingBuffer<int16_t, 13230> buffer_L = {}, buffer_R = {};
    float time = {};
    float level = {};
    float feedback = {};
    uint32_t delay_length = {};

    uint32_t getTotalSamples();

public:
    void reset();
    void setDelay(float time = 0.25f, float level = 0.3f, float feedback = 0.5f);
    int16_t process(int16_t in, bool isR);
    uint32_t getDelayLength();
};