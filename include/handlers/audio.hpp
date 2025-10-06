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

class AudioHandler {
private:
// --------------
// Audio Object
// --------------
    // I2S 入力 -> RecordQueue(Left, Right)
    AudioInputI2S    i2s   = {}; // BCLK=21, MCLK=23, LRCLK=20, RX=8
    AudioRecordQueue rec_L = {}; // L
    AudioRecordQueue rec_R = {}; // R

    // I2SQuad 出力 (4ch: L+, L-, R+, R-)
    AudioOutputI2SQuad i2s_quad = {}; // BCLK=21, MCLK=23, LRCLK=20, TX(1+2)=7, TX(3+4)=32
    AudioPlayQueue     queue_L  = {}; // L+
    AudioPlayQueue     queue_R  = {}; // L- (Lの符号反転)
    AudioPlayQueue     queue_LM = {}; // R+
    AudioPlayQueue     queue_RM = {}; // R- (Rの符号反転)

// --------------
// Audio Connection
// --------------
    // I2S 入力 -> RecordQueue(Left, Right)
    AudioConnection  patchCord5 = {i2s, 0, rec_L, 0};
    AudioConnection  patchCord6 = {i2s, 1, rec_R, 0};

    // PlayQueue -> I2SQuad 出力  (チャネル順: 0=L+, 1=L-, 2=R+, 3=R-)
    AudioConnection  patchCord1 = {queue_L,  0, i2s_quad, 0};
    AudioConnection  patchCord2 = {queue_LM, 0, i2s_quad, 1};
    AudioConnection  patchCord3 = {queue_R,  0, i2s_quad, 2};
    AudioConnection  patchCord4 = {queue_RM, 0, i2s_quad, 3};

    State& state_;
    void init();

public:
    AudioHandler(State& state) : state_(state) {
        init();
    }
    void process();
    void beginRecord();
    void endRecord();
};