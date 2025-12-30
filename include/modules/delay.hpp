#pragma once

#include "handlers/audio.hpp"
#include "utils/buffer.hpp"

constexpr uint32_t DELAY_BUFFER_MS = 300;
constexpr uint32_t DELAY_BUFFER_SIZE = (DELAY_BUFFER_MS * SAMPLE_RATE) / 1000;
constexpr uint8_t  FIXED_POINT_SHIFT = 10;

constexpr int32_t MIN_TIME = 1;
constexpr int32_t MAX_TIME = 300;
constexpr int32_t MIN_LEVEL = 0;
constexpr int32_t MAX_LEVEL = 1024;
constexpr int32_t MIN_FEEDBACK = 0;
constexpr int32_t MAX_FEEDBACK = 1024;

class Delay {
private:
    // 13230samples = 44100hz * 300ms
    IntervalRingBuffer<int16_t, DELAY_BUFFER_SIZE> buffer_L = {}, buffer_R = {};

    int32_t time = 256;
    int32_t level = 307;
    int32_t feedback = 512;
    uint32_t delay_length = 0;

    inline float getFeedbackRatio() const {
        return static_cast<float>(feedback) / static_cast<float>(MAX_FEEDBACK);
    }

    uint32_t getTotalSamples() const;

    static inline int32_t clamp_param(int32_t value, int32_t min, int32_t max) {
        return std::clamp<int32_t>(value, min, max);
    }

public:
    void reset();
    void setDelay(int32_t time = 256, int32_t level = 307, int32_t feedback = 512);
    int16_t processL(int16_t in);
    int16_t processR(int16_t in);
    uint32_t getDelayLength() const;

    // パラメータ取得
    int32_t getTime() const { return time; }
    int32_t getLevel() const { return level; }
    int32_t getFeedback() const { return feedback; }
};