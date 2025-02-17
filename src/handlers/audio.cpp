#include "handlers/audio.hpp"

int16_t samples_L[BUFFER_SIZE];
int16_t samples_R[BUFFER_SIZE];
int16_t samples_LM[BUFFER_SIZE];
int16_t samples_RM[BUFFER_SIZE];
bool samples_ready = false;

/** @brief オーディオハンドラ初期化処理 */
void AudioHandler::init() {
    queue_L.setMaxBuffers(QUEUE_BLOCKS);
    queue_R.setMaxBuffers(QUEUE_BLOCKS);
    queue_LM.setMaxBuffers(QUEUE_BLOCKS);
    queue_RM.setMaxBuffers(QUEUE_BLOCKS);
    // QUEUE_BLOCKS*queue + record_queue + 2(予備)
    AudioMemory(QUEUE_BLOCKS*6 + 4 + 2);
}

/** @brief オーディオ処理 */
void AudioHandler::process() {
    auto& led_audio = State::led_audio;

    if(samples_ready
        && queue_L.available() && queue_R.available()
        && queue_LM.available() && queue_RM.available())
    {
        led_audio = true;
        queue_L.play(samples_L, BUFFER_SIZE);
        queue_R.play(samples_R, BUFFER_SIZE);
        queue_LM.play(samples_LM, BUFFER_SIZE);
        queue_RM.play(samples_RM, BUFFER_SIZE);
        samples_ready = false;
    }
    else {
        led_audio = false;
    }
}