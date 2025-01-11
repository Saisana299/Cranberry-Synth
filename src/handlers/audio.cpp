#include <handlers/audio.h>

AudioSynthWaveformSine sine1;
// AudioPlayQueue queue_outL, queue_outR;
AudioOutputI2S2 i2s2;
AudioConnection patchCord1(sine1, 0, i2s2, 1);
AudioConnection patchCord2(sine1, 0, i2s2, 0);
// AudioConnection patchCord1(queue_outR, 0, i2s2, 1);
// AudioConnection patchCord2(queue_outL, 0, i2s2, 0);

void AudioHandler::init() {
    //patchCord1 = AudioConnection(queue_outR, 0, i2s2, 1);
    //patchCord2 = AudioConnection(queue_outL, 0, i2s2, 0);
    AudioMemory(12);
    sine1.frequency(1000);
    sine1.amplitude(0.5);
}

void AudioHandler::update() {
    // memcpy(queue_outL.getBuffer(), buffer              , BUFFER_SIZE * sizeof(int16_t));
    // memcpy(queue_outR.getBuffer(), buffer + BUFFER_SIZE, BUFFER_SIZE * sizeof(int16_t));
    // queue_outR.playBuffer();
    // queue_outL.playBuffer();
}

void AudioHandler::process() {
    // if(queue_outL.available() && queue_outR.available()) {
    //     update();
    // }
}
