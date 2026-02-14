#pragma once

#include "ui/ui.hpp"
#include "modules/passthrough.hpp"
#include "ui/screens/passthrough_fx.hpp"

// main.cpp で定義された Passthrough インスタンス
extern Passthrough passthrough;

class PassthroughScreen : public Screen {
private:
    const int16_t HEADER_H = 14;
    const int16_t FLOW_Y = 26;       // 信号フロー表示の Y 開始位置
    const int16_t FX_Y = 58;         // エフェクト項目の Y 開始位置
    const int16_t FX_ITEM_H = 16;    // エフェクト項目の高さ
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 14;

    bool needsFullRedraw = false;

    // アニメーション用
    uint32_t lastAnimMs = 0;
    uint8_t animFrame = 0;
    static constexpr uint32_t ANIM_INTERVAL = 500;

    // カーソル位置
    enum CursorPos {
        C_LPF = 0,
        C_HPF,
        C_DELAY,
        C_MAX
    };
    int8_t cursor = C_LPF;

    bool pushingSubscreen_ = false;

public:
    PassthroughScreen() = default;

    void onEnter(UIManager* manager) override {
        this->manager = manager;

        if (!passthrough.isActive()) {
            State& state = manager->getState();
            state.setModeState(MODE_PASSTHROUGH);
            passthrough.begin();
            cleaned_ = false;
        }

        needsFullRedraw = true;
        animFrame = 0;
        manager->invalidate();
        manager->triggerFullTransfer();
    }

    void onExit() override {
        if (!pushingSubscreen_) {
            cleanup();
        }
        pushingSubscreen_ = false;
    }

    bool isAnimated() const override { return true; }

    void handleInput(uint8_t button) override {
        if (button == BTN_CXL) {
            cleanup();
            manager->popScreen();
            return;
        }

        bool moved = false;

        // カーソル移動（上下）
        if (button == BTN_DN || button == BTN_DN_LONG) {
            cursor++;
            if (cursor >= C_MAX) cursor = 0;
            moved = true;
        }
        else if (button == BTN_UP || button == BTN_UP_LONG) {
            cursor--;
            if (cursor < 0) cursor = C_MAX - 1;
            moved = true;
        }

        else if (button == BTN_ET) {
            pushingSubscreen_ = true;
            switch (cursor) {
                case C_LPF:
                    manager->pushScreen(new PassthroughLPFScreen());
                    return;
                case C_HPF:
                    manager->pushScreen(new PassthroughHPFScreen());
                    return;
                case C_DELAY:
                    manager->pushScreen(new PassthroughDelayScreen());
                    return;
            }
            pushingSubscreen_ = false;
        }

        if (moved) {
            manager->invalidate();
        }
    }

    void draw(GFXcanvas16& canvas) override {
        static bool firstDraw = true;
        static int8_t lastCursor = -1;

        if (needsFullRedraw) {
            firstDraw = true;
            lastCursor = -1;
            needsFullRedraw = false;
        }

        if (firstDraw) {
            canvas.fillScreen(Color::BLACK);

            // ヘッダー
            canvas.setTextSize(1);
            canvas.setTextColor(Color::WHITE);
            canvas.setCursor(2, 3);
            canvas.print("PASSTHROUGH MODE");
            canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);

            // 信号フロー表示
            drawSignalFlow(canvas);

            // 区切り線
            canvas.drawFastHLine(0, FX_Y - 4, SCREEN_WIDTH, Color::MD_GRAY);

            // エフェクト項目
            drawAllFxItems(canvas);

            // フッター
            drawFooter(canvas);

            firstDraw = false;
            lastCursor = cursor;
            manager->triggerFullTransfer();
        }

        // カーソル移動時の部分更新
        if (cursor != lastCursor) {
            if (lastCursor >= 0) drawFxItem(canvas, lastCursor, false);
            drawFxItem(canvas, cursor, true);
            lastCursor = cursor;
        }

        // アニメーション更新
        uint32_t now = millis();
        if (now - lastAnimMs >= ANIM_INTERVAL) {
            lastAnimMs = now;
            animFrame = (animFrame + 1) % 4;
            drawSignalIndicator(canvas);
        }
    }

private:
    bool cleaned_ = false;

    void cleanup() {
        if (cleaned_) return;
        passthrough.end();
        manager->getState().setModeState(MODE_SYNTH);
        cleaned_ = true;
    }

    // --- 描画関数群 ---

    void drawSignalFlow(GFXcanvas16& canvas) {
        int16_t centerY = FLOW_Y + 14;
        int16_t boxW = 36;
        int16_t boxH = 16;
        int16_t inX = 6;
        int16_t outX = SCREEN_WIDTH - boxW - 6;

        // デバイス名（ボックスの上）
        canvas.setTextSize(1);
        canvas.setTextColor(Color::MD_GRAY);
        canvas.setCursor(inX, centerY - boxH / 2 - 10);
        canvas.print("PCM1802");
        canvas.setCursor(outX, centerY - boxH / 2 - 10);
        canvas.print("PCM5102");

        // ADC ボックス
        canvas.drawRect(inX, centerY - boxH / 2, boxW, boxH, Color::MD_TEAL);
        canvas.setTextColor(Color::MD_TEAL);
        canvas.setCursor(inX + 6, centerY - 3);
        canvas.print("ADC");

        // 矢印
        int16_t arrowStartX = inX + boxW + 3;
        int16_t arrowEndX = outX - 3;
        canvas.drawFastHLine(arrowStartX, centerY, arrowEndX - arrowStartX, Color::MD_GRAY);
        canvas.drawLine(arrowEndX - 4, centerY - 3, arrowEndX, centerY, Color::MD_GRAY);
        canvas.drawLine(arrowEndX - 4, centerY + 3, arrowEndX, centerY, Color::MD_GRAY);

        // DAC ボックス
        canvas.drawRect(outX, centerY - boxH / 2, boxW, boxH, Color::CRANBERRY);
        canvas.setTextColor(Color::CRANBERRY);
        canvas.setCursor(outX + 6, centerY - 3);
        canvas.print("DAC");
    }

    void drawSignalIndicator(GFXcanvas16& canvas) {
        int16_t centerX = SCREEN_WIDTH / 2;
        int16_t centerY = FLOW_Y + 14;

        canvas.fillRect(centerX - 12, centerY - 4, 24, 9, Color::BLACK);
        uint16_t dotColor = Color::MD_GREEN;
        int16_t dotX = centerX - 8 + (animFrame * 5);
        canvas.fillCircle(dotX, centerY, 2, dotColor);
        manager->transferPartial(centerX - 12, centerY - 4, 24, 9);
    }

    void drawAllFxItems(GFXcanvas16& canvas) {
        for (int i = 0; i < C_MAX; i++) {
            drawFxItem(canvas, i, (cursor == i));
        }
    }

    void drawFxItem(GFXcanvas16& canvas, int8_t idx, bool selected) {
        if (idx < 0 || idx >= C_MAX) return;

        int16_t y = FX_Y + idx * FX_ITEM_H;

        const char* label = nullptr;
        bool enabled = false;

        switch (idx) {
            case C_LPF:
                label = "LPF";
                enabled = passthrough.isLpfEnabled();
                break;
            case C_HPF:
                label = "HPF";
                enabled = passthrough.isHpfEnabled();
                break;
            case C_DELAY:
                label = "DELAY";
                enabled = passthrough.isDelayEnabled();
                break;
        }
        if (!label) return;

        // 背景クリア（行全体）
        canvas.fillRect(0, y, SCREEN_WIDTH, FX_ITEM_H, Color::BLACK);

        canvas.setTextSize(1);

        // 選択時インジケータ（白いバー）
        if (selected) {
            canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        }

        // 有効状態インジケータ（シアンの点）
        if (enabled) {
            canvas.fillCircle(10, y + 6, 2, Color::CYAN);
        }

        // ラベル
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(18, y + 4);
        canvas.print(label);

        manager->transferPartial(0, y, SCREEN_WIDTH, FX_ITEM_H);
    }

    void drawFooter(GFXcanvas16& canvas) {
        canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
        canvas.setTextSize(1);
        canvas.setTextColor(Color::MD_GRAY);
        canvas.setCursor(4, FOOTER_Y + 4);
        canvas.print("ET:EDIT  CXL:EXIT");
    }
};
