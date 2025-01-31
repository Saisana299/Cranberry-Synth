#pragma once

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "utils/state.hpp"

// サンプルバッファと状態は公開
extern int16_t samples_L[];
extern int16_t samples_R[];
extern bool    samples_ready;

constexpr float   SAMPLE_RATE  = 44100.0f;
constexpr size_t  BUFFER_SIZE  = 128;
constexpr uint8_t QUEUE_BLOCKS = 2;

class AudioHandler {
private:
    AudioPlayQueue  queue_L = {}, queue_R = {};
    AudioOutputI2S2 i2s2 = {};
    AudioConnection patchCord1 = {queue_L, 0, i2s2, 0};
    AudioConnection patchCord2 = {queue_R, 0, i2s2, 1};
    void init();

public:
    AudioHandler() {
        init();
    }
    void process();
};