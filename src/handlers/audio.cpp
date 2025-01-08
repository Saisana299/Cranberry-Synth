#include <handlers/audio.h>

void AudioHandler::init() {
    patchCord1 = AudioConnection(queue_outR, 0, i2s1, 1);
    patchCord2 = AudioConnection(queue_outL, 0, i2s1, 0);
    AudioMemory(40);
}

void AudioHandler::update() {
    memcpy(queue_outL.getBuffer(), buffer              , BUFFER_SIZE * sizeof(int16_t));
    memcpy(queue_outR.getBuffer(), buffer + BUFFER_SIZE, BUFFER_SIZE * sizeof(int16_t));
    queue_outR.playBuffer();
    queue_outL.playBuffer();
}

void AudioHandler::process() {
    if(queue_outL.available() && queue_outR.available()) {
        update();
    }
}
