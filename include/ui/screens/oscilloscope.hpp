#pragma once

#include "ui/ui.hpp"
#include "handlers/audio.hpp"
#include "modules/synth.hpp"
#include <algorithm>
#include <cstring>

/**
 * @brief オシロスコープ画面
 * オーディオ出力波形をリアルタイム表示する。
 * L/R/L+R の3モード切り替え、フリーズ（一時停止）、
 * ゼロクロストリガーによる安定表示に対応。
 */
class OscilloscopeScreen : public Screen {
private:
    // === レイアウト定数 ===
    static constexpr int16_t HEADER_H     = 12;
    static constexpr int16_t FOOTER_H     = 12;
    static constexpr int16_t WAVE_TOP     = HEADER_H + 1;
    static constexpr int16_t WAVE_BOTTOM  = SCREEN_HEIGHT - FOOTER_H - 1;
    static constexpr int16_t WAVE_HEIGHT  = WAVE_BOTTOM - WAVE_TOP;
    static constexpr int16_t WAVE_CENTER  = WAVE_TOP + WAVE_HEIGHT / 2;

    // サンプル数: 96（トリガー探索分を差し引いても常にデータが足りる）
    static constexpr int16_t SAMPLE_COUNT = BUFFER_SIZE - BUFFER_SIZE / 4; // 96

    // === 表示モード ===
    enum DisplayMode : uint8_t {
        MODE_LR = 0,   // L+R 同時表示
        MODE_L,        // L のみ
        MODE_R,        // R のみ
        MODE_COUNT
    };

    DisplayMode displayMode = MODE_LR;
    bool frozen = false;

    // 波形バッファ（描画用）
    int16_t waveL[SAMPLE_COUNT];
    int16_t waveR[SAMPLE_COUNT];
    int16_t prevWaveL[SAMPLE_COUNT];
    int16_t prevWaveR[SAMPLE_COUNT];
    bool hasData = false;

    // トリガー探索範囲（先頭の最大25%で探す、残り75%以上を描画に確保）
    static constexpr int16_t MAX_TRIGGER_SEARCH = BUFFER_SIZE / 4; // 32

    // 手動ゲイン
    static constexpr uint8_t GAIN_STEPS = 7;
    static constexpr int16_t GAIN_TABLE[GAIN_STEPS] = {1, 2, 4, 6, 8, 12, 16};
    uint8_t gainIndex = 0;  // デフォルト x1

    // Y座標変換: 固定スケーリング + 手動ゲイン
    inline int16_t sampleToY(int16_t sample) const {
        int16_t halfH = WAVE_HEIGHT / 2;
        int32_t scaled = (static_cast<int32_t>(sample) * halfH * GAIN_TABLE[gainIndex]) / 32767;
        return static_cast<int16_t>(std::clamp<int32_t>(WAVE_CENTER - scaled, WAVE_TOP, WAVE_BOTTOM));
    }

    /**
     * @brief ゼロクロストリガー（スナップショット内で検索）
     * 先頭 MAX_TRIGGER_SEARCH サンプル内で負→正の交差を探しオフセットを返す。
     * 見つからなければ 0（フリーラン）。
     */
    int16_t findTriggerOffset(const Sample16_t* buf) const {
        for (int16_t i = 1; i < MAX_TRIGGER_SEARCH; i++) {
            if (buf[i - 1] < 0 && buf[i] >= 0) {
                return i;
            }
        }
        return 0;
    }

    /**
     * @brief 波形取得
     * 128サンプルのスナップショットを取り、トリガー後の96サンプルを描画用バッファにコピー。
     * 常にちょうど96点分の連続データが保証される。
     */
    void captureWaveform() {
        // 前フレームのデータを保存
        memcpy(prevWaveL, waveL, sizeof(waveL));
        memcpy(prevWaveR, waveR, sizeof(waveR));

        // スナップショット（128サンプル一括コピー）
        Sample16_t snapL[BUFFER_SIZE];
        Sample16_t snapR[BUFFER_SIZE];
        memcpy(snapL, samples_L, sizeof(snapL));
        memcpy(snapR, samples_R, sizeof(snapR));

        // トリガーポイントを検出
        const Sample16_t* trigBuf = snapL;
        if (displayMode == MODE_R) trigBuf = snapR;
        int16_t offset = findTriggerOffset(trigBuf);

        // オフセットから96点コピー（常に offset + 96 <= 128）
        for (int16_t i = 0; i < SAMPLE_COUNT; i++) {
            waveL[i] = snapL[i + offset];
            waveR[i] = snapR[i + offset];
        }

        hasData = true;
    }

    /**
     * @brief 線形補間でサンプル値を取得
     * 96サンプルを128ピクセルに引き伸ばすため、
     * 浮動小数点インデックスから前後のサンプルを補間する。
     */
    inline int16_t interpolateSample(const int16_t* wave, int32_t idx_x256) const {
        int16_t idx = static_cast<int16_t>(idx_x256 >> 8);
        int16_t frac = static_cast<int16_t>(idx_x256 & 0xFF);
        if (idx >= SAMPLE_COUNT - 1) return wave[SAMPLE_COUNT - 1];
        int32_t a = wave[idx];
        int32_t b = wave[idx + 1];
        return static_cast<int16_t>(a + ((b - a) * frac >> 8));
    }

    // === 波形描画（96サンプル → 128px に引き伸ばし）===
    void drawWaveform(GFXcanvas16& canvas, const int16_t* wave, uint16_t color) {
        // ステップ: (SAMPLE_COUNT-1) / (SCREEN_WIDTH-1) を固定小数点(8bit)で
        // = 95 * 256 / 127 ≈ 191.37 → 191
        static constexpr int32_t STEP_X256 = ((SAMPLE_COUNT - 1) * 256) / (SCREEN_WIDTH - 1);

        int32_t sampleIdx = 0;
        int16_t prevY = sampleToY(interpolateSample(wave, 0));

        for (int16_t x = 1; x < SCREEN_WIDTH; x++) {
            sampleIdx = static_cast<int32_t>(x) * STEP_X256;
            int16_t y = sampleToY(interpolateSample(wave, sampleIdx));
            canvas.drawLine(x - 1, prevY, x, y, color);
            prevY = y;
        }
    }

    // === ヘッダー描画 ===
    void drawHeader(GFXcanvas16& canvas) {
        canvas.fillRect(0, 0, SCREEN_WIDTH, HEADER_H, Color::BLACK);
        canvas.setTextSize(1);
        canvas.setTextColor(Color::WHITE);
        canvas.setCursor(2, 2);
        canvas.print("SCOPE");

        // チャンネル表示
        canvas.setCursor(42, 2);
        switch (displayMode) {
            case MODE_LR:
                canvas.setTextColor(Color::CYAN);
                canvas.print("L");
                canvas.setTextColor(Color::MD_GRAY);
                canvas.print("+");
                canvas.setTextColor(Color::GREEN);
                canvas.print("R");
                break;
            case MODE_L:
                canvas.setTextColor(Color::CYAN);
                canvas.print("L");
                break;
            case MODE_R:
                canvas.setTextColor(Color::GREEN);
                canvas.print("R");
                break;
            default: break;
        }

        // フリーズ表示
        if (frozen) {
            canvas.setTextColor(Color::MD_RED);
            canvas.setCursor(90, 2);
            canvas.print("FRZ");
        }

        // ゲイン表示
        canvas.setTextColor(Color::MD_YELLOW);
        canvas.setCursor(110, 2);
        canvas.print("x");
        canvas.print(GAIN_TABLE[gainIndex]);

        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::DARK_SLATE);
    }

    // === フッター描画 ===
    void drawFooter(GFXcanvas16& canvas) {
        int16_t footerY = SCREEN_HEIGHT - FOOTER_H;
        canvas.fillRect(0, footerY, SCREEN_WIDTH, FOOTER_H, Color::BLACK);
        canvas.drawFastHLine(0, footerY, SCREEN_WIDTH, Color::DARK_SLATE);

        canvas.setTextSize(1);
        canvas.setTextColor(Color::MD_GRAY);
        canvas.setCursor(2, footerY + 2);
        canvas.print("\x18\x19:CH \x1b\x1a:AMP");
        canvas.setCursor(90, footerY + 2);
        canvas.print("ET:FRZ");
    }

public:
    OscilloscopeScreen() {
        memset(waveL, 0, sizeof(waveL));
        memset(waveR, 0, sizeof(waveR));
        memset(prevWaveL, 0, sizeof(prevWaveL));
        memset(prevWaveR, 0, sizeof(prevWaveR));
    }

    void onEnter(UIManager* manager) override {
        this->manager = manager;
        frozen = false;
        hasData = false;
        manager->invalidate();
        manager->triggerFullTransfer();
    }

    bool isAnimated() const override { return true; }

    void handleInput(uint8_t button) override {
        // UP/DN: チャンネル切り替え
        if (button == BTN_UP || button == BTN_DN) {
            displayMode = static_cast<DisplayMode>(
                (static_cast<uint8_t>(displayMode) + 1) % MODE_COUNT
            );
            manager->invalidate();
        }
        // ENTER: フリーズ/解除
        else if (button == BTN_ET) {
            frozen = !frozen;
            manager->invalidate();
        }
        // LEFT: ゲイン下げ
        else if (button == BTN_L || button == BTN_L_LONG) {
            if (gainIndex > 0) gainIndex--;
            manager->invalidate();
        }
        // RIGHT: ゲイン上げ
        else if (button == BTN_R || button == BTN_R_LONG) {
            if (gainIndex < GAIN_STEPS - 1) gainIndex++;
            manager->invalidate();
        }
        // CANCEL: 戻る
        else if (button == BTN_CXL) {
            manager->popScreen();
            return;
        }
    }

    void draw(GFXcanvas16& canvas) override {
        // フリーズ中でなければ波形を取得
        if (!frozen) {
            captureWaveform();
        }

        // 背景クリア（波形領域のみ）
        canvas.fillRect(0, WAVE_TOP, SCREEN_WIDTH, WAVE_HEIGHT + 1, Color::BLACK);

        // グリッド線（中心線 + 1/4線）
        canvas.drawFastHLine(0, WAVE_CENTER, SCREEN_WIDTH, Color::DARK_SLATE);
        int16_t quarterH = WAVE_HEIGHT / 4;
        for (int16_t i = 0; i < SCREEN_WIDTH; i += 4) {
            canvas.drawPixel(i, WAVE_CENTER - quarterH, Color::CHARCOAL);
            canvas.drawPixel(i, WAVE_CENTER + quarterH, Color::CHARCOAL);
        }

        // 波形描画
        if (hasData) {
            switch (displayMode) {
                case MODE_LR:
                    drawWaveform(canvas, waveR, Color::GREEN);
                    drawWaveform(canvas, waveL, Color::CYAN);
                    break;
                case MODE_L:
                    drawWaveform(canvas, waveL, Color::CYAN);
                    break;
                case MODE_R:
                    drawWaveform(canvas, waveR, Color::GREEN);
                    break;
                default: break;
            }
        }

        // ヘッダー・フッター
        drawHeader(canvas);
        drawFooter(canvas);

        // 全画面転送
        manager->triggerFullTransfer();
    }
};
