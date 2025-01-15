#ifndef AUDIO_HANDLER_H
#define AUDIO_HANDLER_H

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

extern int16_t samples_L[];
extern int16_t samples_R[];
extern bool samples_ready;

#define SAMPLE_RATE 44100.0
#define BLOCK_SIZE 128

class AudioHandler {
private:
    AudioPlayQueue queue_L, queue_R;
    AudioOutputI2S2 i2s2;
    AudioConnection patchCord1, patchCord2;
    float phase, delta;
    int counter;
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