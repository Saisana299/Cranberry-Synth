#pragma once

#include "handlers/audio.hpp"
#include "utils/buffer.hpp"

class Delay {
private:
    // 13230samples = 44100hz * 300ms
    IntervalRingBuffer<int16_t, 13230> buffer_L = {}, buffer_R = {};
    int32_t time = {};
    int32_t level = {};
    int32_t feedback = {};
    uint32_t delay_length = {};

    uint32_t getTotalSamples();

public:
    void reset();
    void setDelay(int32_t time = 256, int32_t level = 307, int32_t feedback = 512);
    int16_t process(int16_t in, bool isR);
    uint32_t getDelayLength();
};