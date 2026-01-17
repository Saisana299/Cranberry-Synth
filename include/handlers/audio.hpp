#pragma once

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <atomic>

#include "types.hpp"
#include "utils/state.hpp"

// --------------
// Audio Buffer
// --------------
extern Sample16_t samples_L[];   // L
extern Sample16_t samples_R[];   // R
extern Sample16_t samples_LM[];  // L (位相反転)
extern Sample16_t samples_RM[];  // R (位相反転)
extern std::atomic<bool> samples_ready_flags; // サンプルの送信準備が完了したか

// --------------
// Audio Setting
// --------------
constexpr int32_t  SAMPLE_RATE  = 44100; // サンプリング周波数
constexpr size_t   BUFFER_SIZE  = 128;   // AudioBufferのサイズ
constexpr uint8_t  QUEUE_BLOCKS = 2;     // AudioPlayQueueのバッファ数
constexpr uint32_t AUDIO_MEMORY = QUEUE_BLOCKS * 6 + 4 + 2; // AudioMemoryの必要量

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
    AudioConnection  patchCord1 = {i2s, 0, rec_L, 0};
    AudioConnection  patchCord2 = {i2s, 1, rec_R, 0};

    // PlayQueue -> I2SQuad 出力  (チャネル順: 0=L+, 1=L-, 2=R+, 3=R-)
    AudioConnection  patchCord3 = {queue_L,  0, i2s_quad, 0};
    AudioConnection  patchCord4 = {queue_LM, 0, i2s_quad, 1};
    AudioConnection  patchCord5 = {queue_R,  0, i2s_quad, 2};
    AudioConnection  patchCord6 = {queue_RM, 0, i2s_quad, 3};

    State& state_;

public:
    AudioHandler(State& state) : state_(state) {}

    void init();

    /** @brief バッファに格納されたオーディオデータを再生 */
    void process();

    /** @brief 録音開始 */
    void beginRecord();

    /** @brief 録音終了 */
    void endRecord();
};