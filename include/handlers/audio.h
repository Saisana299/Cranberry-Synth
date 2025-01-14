#ifndef AUDIO_HANDLER_H
#define AUDIO_HANDLER_H

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

extern AudioPlayQueue queue_L, queue_R;
extern AudioOutputI2S2 i2s2;
extern AudioConnection patchCord1;
extern AudioConnection patchCord2;

class AudioHandler {
private:
    float phase;
    float delta;

    static constexpr float SAMPLE_RATE = 44100.0;
    static constexpr int   BLOCK_SIZE = 128;

    int16_t samples_L[BLOCK_SIZE];
    int16_t samples_R[BLOCK_SIZE];

    void init();
    void update();
    int16_t triangle(float phase);

public:
    AudioHandler() {
        init();
    }
    void process();
};

#endif