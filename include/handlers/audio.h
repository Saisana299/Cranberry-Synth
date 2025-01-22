#pragma once

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "utils/state.h"

extern int16_t samples_L[];
extern int16_t samples_R[];
extern bool samples_ready;

#define SAMPLE_RATE 44100.0
#define BUFFER_SIZE 128
#define QUEUE_BLOCKS 2

class AudioHandler {
private:
    AudioPlayQueue queue_L, queue_R;
    AudioOutputI2S2 i2s2;
    AudioConnection patchCord1, patchCord2;
    void init();

public:
    AudioHandler() {
        init();
    }
    void process();
};