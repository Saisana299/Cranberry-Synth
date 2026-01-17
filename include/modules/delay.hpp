#pragma once

#include "types.hpp"
#include "handlers/audio.hpp"
#include "utils/buffer.hpp"

constexpr uint32_t DELAY_BUFFER_MS = 300;
constexpr uint32_t DELAY_BUFFER_SIZE = (DELAY_BUFFER_MS * SAMPLE_RATE) / 1000;

constexpr int32_t MIN_TIME = 1;
constexpr int32_t MAX_TIME = 300;
constexpr Gain_t MIN_LEVEL = 0;
constexpr Gain_t MAX_LEVEL = Q15_MAX;
constexpr Gain_t MIN_FEEDBACK = 0;
constexpr Gain_t MAX_FEEDBACK = Q15_MAX;

class Delay {
private:
    // 13230samples = 44100hz * 300ms
    IntervalRingBuffer<Sample16_t, DELAY_BUFFER_SIZE> buffer_L = {}, buffer_R = {};

    int32_t time = 80;          // ms (1-300)
    Gain_t level = 9830;        // Q15 (default: 30% = 9830)
    Gain_t feedback = 16384;    // Q15 (default: 50% = 16384)
    uint32_t delay_length = 0;

    inline float getFeedbackRatio() const {
        return Q15_to_float(feedback);
    }

    uint32_t getTotalSamples() const;

    static inline Gain_t clamp_gain(Gain_t value, Gain_t min, Gain_t max) {
        return std::clamp<Gain_t>(value, min, max);
    }

public:
    void reset();
    void setDelay(int32_t time, Gain_t level, Gain_t feedback);
    void setTime(int32_t time);
    void setLevel(Gain_t level);
    void setFeedback(Gain_t feedback);
    Sample16_t processL(Sample16_t in);
    Sample16_t processR(Sample16_t in);
    uint32_t getDelayLength() const;

    // パラメータ取得
    int32_t getTime() const { return time; }
    Gain_t getLevel() const { return level; }
    Gain_t getFeedback() const { return feedback; }
};