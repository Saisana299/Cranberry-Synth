#pragma once

#include "ui/ui.hpp"
#include "modules/passthrough.hpp"
#include "ui/screens/passthrough_fx.hpp"

// main.cpp で定義された Passthrough インスタンス
extern Passthrough passthrough;

// ============================================================
// PassthroughFXListScreen — パススルーモード用エフェクト一覧
// ============================================================
class PassthroughFXListScreen : public Screen {
private:
    const int16_t HEADER_H = 14;
    const int16_t FX_Y = 18;
    const int16_t FX_ITEM_H = 13;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 14;

    bool needsFullRedraw = false;

    enum CursorPos {
        C_LPF = 0,
        C_HPF,
        C_DELAY,
        C_CHORUS,
        C_REVERB,
        C_MAX
    };
    int8_t cursor = C_LPF;

public:
    PassthroughFXListScreen() = default;

    void onEnter(UIManager* manager) override {
        this->manager = manager;
        cursor = C_LPF;
        needsFullRedraw = true;
        manager->invalidate();
        manager->triggerFullTransfer();
    }

    bool isAnimated() const override { return false; }

    void handleInput(uint8_t button) override {
        if (button == BTN_CXL) {
            manager->popScreen();
            return;
        }

        bool moved = false;

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
                case C_CHORUS:
                    manager->pushScreen(new PassthroughChorusScreen());
                    return;
                case C_REVERB:
                    manager->pushScreen(new PassthroughReverbScreen());
                    return;
            }
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
            canvas.print("FX");
            canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);

            // エフェクト項目
            drawAllFxItems(canvas);

            // フッター
            canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
            canvas.setTextSize(1);
            canvas.setTextColor(Color::MD_GRAY);
            canvas.setCursor(4, FOOTER_Y + 4);
            canvas.print("ET:EDIT  CXL:BACK");

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
    }

private:
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
            case C_CHORUS:
                label = "CHORUS";
                enabled = passthrough.isChorusEnabled();
                break;
            case C_REVERB:
                label = "REVERB";
                enabled = passthrough.isReverbEnabled();
                break;
        }
        if (!label) return;

        canvas.fillRect(0, y, SCREEN_WIDTH, FX_ITEM_H, Color::BLACK);
        canvas.setTextSize(1);

        if (selected) {
            canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        }

        if (enabled) {
            canvas.fillCircle(10, y + 6, 2, Color::CYAN);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(18, y + 4);
        canvas.print(label);

        // 右矢印でサブメニューを示す
        canvas.setCursor(110, y + 4);
        canvas.setTextColor(Color::MD_GRAY);
        canvas.print(">");

        manager->transferPartial(0, y, SCREEN_WIDTH, FX_ITEM_H);
    }
};

// ============================================================
// PassthroughScreen — パススルーモードメイン画面
// ============================================================
class PassthroughScreen : public Screen {
private:
    const int16_t HEADER_H = 14;
    const int16_t FLOW_Y = 26;       // 信号フロー表示の Y 開始位置
    const int16_t MENU_Y = 58;       // メニュー項目の Y 開始位置
    const int16_t MENU_ITEM_H = 16;  // メニュー項目の高さ
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 14;

    static constexpr Gain_t VOL_STEP_SMALL = Q15_MAX / 100;   // 1%
    static constexpr Gain_t VOL_STEP_LARGE = Q15_MAX / 10;    // 10%

    bool needsFullRedraw = false;

    // アニメーション用
    uint32_t lastAnimMs = 0;
    uint8_t animFrame = 0;
    static constexpr uint32_t ANIM_INTERVAL = 500;

    // カーソル位置
    enum CursorPos {
        C_VOLUME = 0,
        C_FX,
        C_MAX
    };
    int8_t cursor = C_VOLUME;

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

        // 左右ボタン：VOLUME選択時はボリューム調整
        else if (button == BTN_L || button == BTN_L_LONG) {
            if (cursor == C_VOLUME) {
                Gain_t step = (button == BTN_L_LONG) ? VOL_STEP_LARGE : VOL_STEP_SMALL;
                int32_t vol = static_cast<int32_t>(passthrough.getVolume()) - step;
                if (vol < 0) vol = 0;
                passthrough.setVolume(static_cast<Gain_t>(vol));
                needsFullRedraw = true;
                manager->invalidate();
                return;
            }
        }
        else if (button == BTN_R || button == BTN_R_LONG) {
            if (cursor == C_VOLUME) {
                Gain_t step = (button == BTN_R_LONG) ? VOL_STEP_LARGE : VOL_STEP_SMALL;
                int32_t vol = static_cast<int32_t>(passthrough.getVolume()) + step;
                if (vol > Q15_MAX) vol = Q15_MAX;
                passthrough.setVolume(static_cast<Gain_t>(vol));
                needsFullRedraw = true;
                manager->invalidate();
                return;
            }
        }

        else if (button == BTN_ET) {
            if (cursor == C_FX) {
                pushingSubscreen_ = true;
                manager->pushScreen(new PassthroughFXListScreen());
                return;
            }
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
            canvas.drawFastHLine(0, MENU_Y - 4, SCREEN_WIDTH, Color::MD_GRAY);

            // メニュー項目
            drawAllMenuItems(canvas);

            // フッター
            drawFooter(canvas);

            firstDraw = false;
            lastCursor = cursor;
            manager->triggerFullTransfer();
        }

        // カーソル移動時の部分更新
        if (cursor != lastCursor) {
            if (lastCursor >= 0) drawMenuItem(canvas, lastCursor, false);
            drawMenuItem(canvas, cursor, true);
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

    void drawAllMenuItems(GFXcanvas16& canvas) {
        for (int i = 0; i < C_MAX; i++) {
            drawMenuItem(canvas, i, (cursor == i));
        }
    }

    void drawMenuItem(GFXcanvas16& canvas, int8_t idx, bool selected) {
        if (idx < 0 || idx >= C_MAX) return;

        int16_t y = MENU_Y + idx * MENU_ITEM_H;

        const char* label = nullptr;
        const char* detail = nullptr;

        switch (idx) {
            case C_VOLUME: {
                label = "VOLUME";
                static char volBuf[8];
                int pct = static_cast<int>(
                    (static_cast<int32_t>(passthrough.getVolume()) * 100) / Q15_MAX);
                sprintf(volBuf, "%d%%", pct);
                detail = volBuf;
                break;
            }
            case C_FX: {
                label = "FX";
                // 有効なエフェクト数を表示
                static char fxBuf[8];
                int count = 0;
                if (passthrough.isLpfEnabled())    count++;
                if (passthrough.isHpfEnabled())    count++;
                if (passthrough.isDelayEnabled())  count++;
                if (passthrough.isChorusEnabled()) count++;
                sprintf(fxBuf, "%d ON", count);
                detail = fxBuf;
                break;
            }
        }
        if (!label) return;

        // 背景クリア
        canvas.fillRect(0, y, SCREEN_WIDTH, MENU_ITEM_H, Color::BLACK);
        canvas.setTextSize(1);

        // 選択時インジケータ
        if (selected) {
            canvas.fillRect(2, y + 3, 3, 10, Color::WHITE);
        }

        // ラベル
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(label);

        // 詳細値
        if (detail) {
            canvas.setTextColor(Color::MD_GRAY);
            canvas.setCursor(80, y + 4);
            canvas.print(detail);
        }

        // FX項目のみ右矢印でサブメニューを示す
        if (idx == C_FX) {
            canvas.setCursor(118, y + 4);
            canvas.setTextColor(Color::MD_GRAY);
            canvas.print(">");
        }

        manager->transferPartial(0, y, SCREEN_WIDTH, MENU_ITEM_H);
    }

    void drawFooter(GFXcanvas16& canvas) {
        canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
        canvas.setTextSize(1);
        canvas.setTextColor(Color::MD_GRAY);
        canvas.setCursor(4, FOOTER_Y + 4);
        canvas.print("ET:EDIT  CXL:EXIT");
    }
};
