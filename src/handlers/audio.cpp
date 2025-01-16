#include "handlers/audio.h"

int16_t samples_L[BUFFER_SIZE];
int16_t samples_R[BUFFER_SIZE];
bool samples_ready = false;

void AudioHandler::init() {
    queue_L.setMaxBuffers(QUEUE_BLOCKS);
    queue_R.setMaxBuffers(QUEUE_BLOCKS);
    patchCord1.connect(queue_R, 0, i2s2, 1);
    patchCord2.connect(queue_L, 0, i2s2, 0);
    AudioMemory(QUEUE_BLOCKS*2 + 2);
}

void AudioHandler::process() {
    auto& led_state = State::led_state;

    if(samples_ready && queue_L.available() && queue_R.available()) {
        led_state = true;
        queue_L.play(samples_L, BUFFER_SIZE);
        queue_R.play(samples_R, BUFFER_SIZE);
        samples_ready = false;
    }
    else {
        led_state = false;
    }
}