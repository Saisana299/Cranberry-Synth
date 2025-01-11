#ifndef AUDIO_HANDLER_H
#define AUDIO_HANDLER_H

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

extern AudioSynthWaveformSine sine1;
// extern AudioPlayQueue queue_outL, queue_outR;
extern AudioOutputI2S2 i2s2;
extern AudioConnection patchCord1;
extern AudioConnection patchCord2;

class AudioHandler {
private:
    static constexpr float SAMPLE_RATE = 44100.0;
    static constexpr int   BUFFER_SIZE = 128;

    int16_t buffer[BUFFER_SIZE * 2]; // L-R

    void update();  // 更新

public:
    void init();    // 初期化
    void process(); // 処理
};

#endif