#include "handlers/audio.hpp"

int16_t samples_L[BUFFER_SIZE];
int16_t samples_R[BUFFER_SIZE];
int16_t samples_LM[BUFFER_SIZE];
int16_t samples_RM[BUFFER_SIZE];
std::atomic<uint32_t> samples_ready_flags = 0;

/** @brief オーディオハンドラ初期化処理 */
void AudioHandler::init() {
    queue_L.setMaxBuffers(QUEUE_BLOCKS);
    queue_R.setMaxBuffers(QUEUE_BLOCKS);
    queue_LM.setMaxBuffers(QUEUE_BLOCKS);
    queue_RM.setMaxBuffers(QUEUE_BLOCKS);
    AudioMemory(AUDIO_MEMORY);
}

/** @brief バッファに格納されたオーディオデータを再生 */
void AudioHandler::process() {
    if(!samples_ready_flags.load()) return;

    if(queue_L.available() && queue_R.available()
        && queue_LM.available() && queue_RM.available()){
        state_.setLedAudio(true);
        queue_L.play(samples_L, BUFFER_SIZE);
        queue_R.play(samples_R, BUFFER_SIZE);
        queue_LM.play(samples_LM, BUFFER_SIZE);
        queue_RM.play(samples_RM, BUFFER_SIZE);
        samples_ready_flags.store(0);
    }
    else {
        state_.setLedAudio(false);
    }
}

/** @brief 録音開始 */ // todo
void AudioHandler::beginRecord() {
    rec_L.clear();
    rec_R.clear();
    rec_L.begin();
    rec_R.begin();
}

/** @brief 録音終了 */ // todo
void AudioHandler::endRecord() {
    rec_L.end();
    rec_R.end();
}

// RecordQueueからオーディオデータを取り出すコード
// void loop() {
//   // L, R それぞれのキューにオーディオデータが溜まっているか確認
//   if ((rec_L.available() > 0) && (rec_R.available() > 0)) {
//     // それぞれ1ブロック(128サンプル)を取り出す
//     int16_t *blockL = rec_L.readBuffer();
//     int16_t *blockR = rec_R.readBuffer();

//     if (blockL != nullptr && blockR != nullptr) {
//       // blockL, blockR に各128サンプルの音声データがある
//       // 符号反転 (L-, R-) や必要な処理を行い、別バッファへコピー
//       for (int i = 0; i < BLOCK_SIZE; i++) {
//         samples_L [i] = blockL[i];           // L+
//         samples_LM[i] = negation(blockL[i]); // L- (反転)
//         samples_R [i] = blockR[i];           // R+
//         samples_RM[i] = negation(blockR[i]); // R- (反転)
//       }

//       // PlayQueue へ書き込み → Quad 出力へ
//       queue_L.play(samples_L,   BLOCK_SIZE);
//       queue_LM.play(samples_LM, BLOCK_SIZE);
//       queue_R.play(samples_R,   BLOCK_SIZE);
//       queue_RM.play(samples_RM, BLOCK_SIZE);
//     }

//     // 使い終わったらバッファを解放 (重要)
//     rec_L.freeBuffer();
//     rec_R.freeBuffer();
//   }
// }