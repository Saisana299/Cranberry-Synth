#include "modules/passthrough.hpp"

/** @brief パススルーモード開始 */
void Passthrough::begin() {
    if (active_) return;

    auto& rec_L = audio_.getRecL();
    auto& rec_R = audio_.getRecR();

    // RecordQueue をクリアしてから開始
    rec_L.clear();
    rec_R.clear();
    rec_L.begin();
    rec_R.begin();

    // エフェクトの状態をリセット
    filter_.reset();
    delay_.reset();
    chorus_.reset();
    reverb_.reset();

    active_ = true;
}

/** @brief パススルーモード終了 */
void Passthrough::end() {
    if (!active_) return;

    auto& rec_L = audio_.getRecL();
    auto& rec_R = audio_.getRecR();

    // RecordQueue を停止
    rec_L.end();
    rec_R.end();

    // 残留バッファを破棄
    while (rec_L.available() > 0) {
        rec_L.readBuffer();
        rec_L.freeBuffer();
    }
    while (rec_R.available() > 0) {
        rec_R.readBuffer();
        rec_R.freeBuffer();
    }

    // PlayQueue にも無音を1ブロック送って出力をクリア
    auto& queue_L  = audio_.getQueueL();
    auto& queue_LM = audio_.getQueueLM();
    auto& queue_R  = audio_.getQueueR();
    auto& queue_RM = audio_.getQueueRM();

    Sample16_t silence[BUFFER_SIZE] = {};
    queue_L.play(silence,  BUFFER_SIZE);
    queue_LM.play(silence, BUFFER_SIZE);
    queue_R.play(silence,  BUFFER_SIZE);
    queue_RM.play(silence, BUFFER_SIZE);

    active_ = false;
}

/** @brief 入力信号を差動出力へパススルー */
void Passthrough::process() {
    if (!active_) return;

    auto& rec_L   = audio_.getRecL();
    auto& rec_R   = audio_.getRecR();
    auto& queue_L  = audio_.getQueueL();
    auto& queue_LM = audio_.getQueueLM();
    auto& queue_R  = audio_.getQueueR();
    auto& queue_RM = audio_.getQueueRM();

    // L, R 両方のブロックが揃うまで待つ
    if (rec_L.available() <= 0 || rec_R.available() <= 0) return;

    int16_t* blockL = rec_L.readBuffer();
    int16_t* blockR = rec_R.readBuffer();

    if (blockL != nullptr && blockR != nullptr) {
        // 入力を一旦 samples_L / samples_R にコピー
        for (size_t i = 0; i < BUFFER_SIZE; i++) {
            samples_L[i] = blockL[i];
            samples_R[i] = blockR[i];
        }

        // --- エフェクトチェーン ---
        // 1. HPF (ハイパスフィルタ)
        if (hpf_enabled_) {
            for (size_t i = 0; i < BUFFER_SIZE; i++) {
                samples_L[i] = filter_.processHpfL(samples_L[i]);
                samples_R[i] = filter_.processHpfR(samples_R[i]);
            }
        }

        // 2. LPF (ローパスフィルタ)
        if (lpf_enabled_) {
            for (size_t i = 0; i < BUFFER_SIZE; i++) {
                samples_L[i] = filter_.processLpfL(samples_L[i]);
                samples_R[i] = filter_.processLpfR(samples_R[i]);
            }
        }

        // 3. Delay
        if (delay_enabled_) {
            for (size_t i = 0; i < BUFFER_SIZE; i++) {
                samples_L[i] = delay_.processL(samples_L[i]);
                samples_R[i] = delay_.processR(samples_R[i]);
            }
        }

        // 4. Chorus
        if (chorus_enabled_) {
            for (size_t i = 0; i < BUFFER_SIZE; i++) {
                chorus_.process(samples_L[i], samples_R[i]);
            }
        }

        // 5. Reverb
        if (reverb_enabled_) {
            for (size_t i = 0; i < BUFFER_SIZE; i++) {
                reverb_.process(samples_L[i], samples_R[i]);
            }
        }

        // 6. Volume (音量調整)
        if (volume_ < Q15_MAX) {
            for (size_t i = 0; i < BUFFER_SIZE; i++) {
                samples_L[i] = static_cast<Sample16_t>(
                    (static_cast<int32_t>(samples_L[i]) * volume_) >> Q15_SHIFT);
                samples_R[i] = static_cast<Sample16_t>(
                    (static_cast<int32_t>(samples_R[i]) * volume_) >> Q15_SHIFT);
            }
        }

        // --- 無音判定 + 差動出力生成 ---
        int16_t peak = 0;
        for (size_t i = 0; i < BUFFER_SIZE; i++) {
            samples_LM[i] = negation(samples_L[i]);
            samples_RM[i] = negation(samples_R[i]);

            int16_t absL = (samples_L[i] < 0) ? -samples_L[i] : samples_L[i];
            int16_t absR = (samples_R[i] < 0) ? -samples_R[i] : samples_R[i];
            if (absL > peak) peak = absL;
            if (absR > peak) peak = absR;
        }

        queue_L.play(samples_L,   BUFFER_SIZE);
        queue_LM.play(samples_LM, BUFFER_SIZE);
        queue_R.play(samples_R,   BUFFER_SIZE);
        queue_RM.play(samples_RM, BUFFER_SIZE);

        // Audio LED: 無音でなければ点灯
        constexpr int16_t SILENCE_THRESHOLD = 64;
        if (peak > SILENCE_THRESHOLD) {
            audio_.getState().setLedAudio(true);
        }
    }

    // バッファ解放 (必須)
    rec_L.freeBuffer();
    rec_R.freeBuffer();
}
