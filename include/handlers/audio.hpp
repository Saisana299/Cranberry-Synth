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
extern int16_t samples_LM[];
extern int16_t samples_RM[];
extern bool    samples_ready;

constexpr int32_t SAMPLE_RATE  = 44100;
constexpr size_t  BUFFER_SIZE  = 128;
constexpr uint8_t QUEUE_BLOCKS = 2;

// I2S QUAD
// BCLK = 21
// MCLK = 23
// TX (ch1+2) = 7
// TX (ch3+4) = 32
// LRCLK = 20

// I2S2
// BCLK = 4
// MCLK = 33
// RX = 5
// LRCLK = 3

class AudioHandler {
private:
    AudioPlayQueue     queue_L  = {}, queue_R  = {};
    AudioPlayQueue     queue_LM = {}, queue_RM = {};
    AudioOutputI2SQuad i2s_quad = {};
    AudioConnection  patchCord1 = {queue_L,  0, i2s_quad, 0};
    AudioConnection  patchCord2 = {queue_LM, 0, i2s_quad, 1};
    AudioConnection  patchCord3 = {queue_R,  0, i2s_quad, 2};
    AudioConnection  patchCord4 = {queue_RM, 0, i2s_quad, 3};

    AudioRecordQueue rec_L ={}, rec_R = {};
    AudioInputI2S2   i2s2 = {};
    AudioConnection  patchCord5 = {i2s2, 0, rec_L, 0};
    AudioConnection  patchCord6 = {i2s2, 1, rec_R, 0};

    void init();

public:
    AudioHandler() {
        init();
    }
    void process();
};