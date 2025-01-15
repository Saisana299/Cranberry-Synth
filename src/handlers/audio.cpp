#include "handlers/audio.h"

#include "handlers/midi.h"
#include "utils/math.h"
#include "utils/state.h"

int16_t samples_L[BLOCK_SIZE];
int16_t samples_R[BLOCK_SIZE];
bool samples_ready = false;

void AudioHandler::init() {
    patchCord1.connect(queue_R, 0, i2s2, 1);
    patchCord2.connect(queue_L, 0, i2s2, 0);
    queue_L.setMaxBuffers(BLOCK_SIZE);
    queue_R.setMaxBuffers(BLOCK_SIZE);
    AudioMemory(162+2); // max:162
    phase = 0;
    delta = 440.0 / SAMPLE_RATE;
    counter = 0;
}

void AudioHandler::update() {
    for(size_t i = 0; i < BLOCK_SIZE; i++) {
        samples_L[i] = triangle(phase);
        samples_R[i] = triangle(phase);
        phase += delta;
        if (phase >= 1.0) phase -= 1.0;
    }
    if(counter < 1000){
        samples_ready = true;
        counter++;
    }
}

int16_t AudioHandler::triangle(float phase) {
    return static_cast<int16_t>((1.0 - fabs(2.0 * phase - 1.0)) * 32767);
}

void AudioHandler::process() {
    update();
    if(samples_ready && queue_L.available() && queue_R.available()) {
        State::led_state = true;
        queue_L.play(samples_L, BLOCK_SIZE);
        queue_R.play(samples_R, BLOCK_SIZE);
        samples_ready = false;
    }
    else {
        State::led_state = false;
    }
}