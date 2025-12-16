#pragma once

#include "ui/ui.hpp"
#include "ui/screens/preset.hpp"

class TitleScreen : public Screen {
private:
    uint32_t frameCount = 0;

public:

    TitleScreen() = default;

    void onEnter(UIManager* manager) override {
        this->manager = manager;
        State& state = manager->getState();
        state.setModeState(MODE_NONE);
    }

    bool isAnimated() const override { return true; }

    void handleInput(uint8_t button) override {
        if (button == BTN_ET) {
            manager->pushScreen(new PresetScreen());
        }
    }

    void draw(GFXcanvas16& canvas) override {
        canvas.fillScreen(Color::BLACK);

        int w = canvas.width();
        int h = canvas.height();
        int centerY = h / 2;

        // --- レイアウト設定 ---

        // 1. メインタイトル
        canvas.setTextSize(2);
        canvas.setTextWrap(false);

        const char* mainTitle = "CRANBERRY";
        TextBounds mainBounds = GFX_SSD1351::getTextBounds(mainTitle, 0, 0);

        int realW = mainBounds.w * 2;
        int realH = mainBounds.h * 2;
        int mainX = (w - realW) / 2;
        int mainY = centerY - 20;

        // 2. アクセントライン
        int lineY = mainY + realH + 8;
        int waveCenterY = mainY + realH - 2; // 中心Y座標
        int waveWidth = 108;
        float amplitude = 6.5f;
        float frequency = 0.07f;
        float flowSpeed = 3.0f;
        int waveStartX = ((w - waveWidth) / 2) - 1;  // 開始X座標

        // 前回の点の座標を記憶する変数
        int lastX = -1;
        int lastY = -1;

        // --- 描画ループ ---
        for (int i = 0; i <= waveWidth; ++i) {
            int x = waveStartX + i;

            // Y座標の計算
            float angle = (i + frameCount * flowSpeed) * frequency;
            int y = waveCenterY + static_cast<int>(std::sin(angle) * amplitude);

            // 最初の点以外なら、前の点から現在の点へ線を引く
            if (i > 0) {
                canvas.drawLine(lastX, lastY, x, y, Color::CRANBERRY);
            }

            // 現在の点を、次回の「前の点」として記憶
            lastX = x;
            lastY = y;
        }

        // アクセントラインの描画後にメインタイトルを描画する
        GFX_SSD1351::drawString(canvas, mainTitle, mainX, mainY, Color::WHITE);

        // 3. サブタイトル
        canvas.setTextSize(1); // サイズを1に戻す
        const char* subTitle = "FM SYNTHESIZER";
        TextBounds subBounds = GFX_SSD1351::getTextBounds(subTitle, 0, 0);
        int subX = (w - subBounds.w) / 2;
        int subY = lineY;

        GFX_SSD1351::drawString(canvas, subTitle, subX, subY, Color::MD_GRAY);

        // 4. スタートプロンプト
        if ((frameCount / 30) % 2 == 0) {
            const char* prompt = "-PRESS ENTER-";
            // 描画処理
             TextBounds pBounds = GFX_SSD1351::getTextBounds(prompt, 0, 0);
             int pX = (w - pBounds.w) / 2;
             int pY = h - 24;
             GFX_SSD1351::drawString(canvas, prompt, pX, pY, Color::MD_GRAY);
        }

        frameCount++;
    }
};