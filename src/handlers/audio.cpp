#include <handlers/audio.h>

AudioPlayQueue queue_L, queue_R;
AudioOutputI2S2 i2s2;
AudioConnection patchCord1(queue_R, 0, i2s2, 1);
AudioConnection patchCord2(queue_L, 0, i2s2, 0);

void AudioHandler::init() {
    phase = 0;
    delta = 440.0 / SAMPLE_RATE;
    AudioMemory(40);
}

void AudioHandler::update() {
    for(size_t i = 0; i < BLOCK_SIZE; i++) {
        samples_L[i] = triangle(phase);
        samples_R[i] = triangle(phase);
        phase += delta;
        if (phase >= 1.0) phase -= 1.0;
    }
    queue_R.play(samples_L, BLOCK_SIZE);
    queue_L.play(samples_R, BLOCK_SIZE);
}

void AudioHandler::process() {
    if(queue_L.available() && queue_R.available()) {
        update();
    }
}

int16_t AudioHandler::triangle(float phase) {
    return static_cast<int16_t>((1.0 - fabs(2.0 * phase - 1.0)) * 32767);
}