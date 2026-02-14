#pragma once

#include "ui/ui.hpp"
#include "ui/screens/passthrough.hpp"

class MenuScreen : public Screen {
private:
    const int16_t HEADER_H = 14;
    const int16_t ITEM_H = 16;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 14;

    bool needsFullRedraw = false;

    enum CursorPos {
        C_PASSTHROUGH = 0,
        C_MAX
    };
    int8_t cursor = C_PASSTHROUGH;

public:
    MenuScreen() = default;

    void onEnter(UIManager* manager) override {
        this->manager = manager;
        needsFullRedraw = true;
        manager->invalidate();
        manager->triggerFullTransfer();
    }

    bool isAnimated() const override { return false; }

    void handleInput(uint8_t button) override {
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

        // ENTERボタン
        else if (button == BTN_ET) {
            if (cursor == C_PASSTHROUGH) {
                manager->pushScreen(new PassthroughScreen());
                return;
            }
        }

        // CANCELボタン：戻る
        else if (button == BTN_CXL) {
            manager->popScreen();
            return;
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

        // --- 初回描画 ---
        if (firstDraw) {
            canvas.fillScreen(Color::BLACK);
            drawHeader(canvas);
            drawAllItems(canvas);
            drawFooter(canvas);

            firstDraw = false;
            lastCursor = cursor;
            manager->triggerFullTransfer();
        }

        // --- カーソル移動時の部分更新 ---
        if (cursor != lastCursor) {
            drawItem(canvas, lastCursor, false);
            drawItem(canvas, cursor, true);
            lastCursor = cursor;
        }
    }

private:
    void drawHeader(GFXcanvas16& canvas) {
        canvas.fillRect(0, 0, SCREEN_WIDTH, HEADER_H, Color::BLACK);
        canvas.setTextSize(1);
        canvas.setTextColor(Color::WHITE);
        canvas.setCursor(2, 3);
        canvas.print("MENU");
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
    }

    void drawAllItems(GFXcanvas16& canvas) {
        for (int i = 0; i < C_MAX; i++) {
            drawItem(canvas, i, (cursor == i));
        }
    }

    void drawItem(GFXcanvas16& canvas, int8_t idx, bool selected) {
        if (idx < 0 || idx >= C_MAX) return;

        int16_t y = HEADER_H + 4 + idx * ITEM_H;
        int16_t x = 6;
        int16_t w = SCREEN_WIDTH - 12;
        int16_t h = ITEM_H - 2;

        // 背景クリア
        canvas.fillRect(x - 2, y - 1, w + 4, h + 2, Color::BLACK);

        const char* label = nullptr;
        const char* desc  = nullptr;

        switch (idx) {
            case C_PASSTHROUGH:
                label = "PASSTHROUGH";
                desc  = "ADC -> DAC Direct";
                break;
        }

        if (label == nullptr) return;

        canvas.setTextSize(1);

        if (selected) {
            // 選択中: 白背景に黒文字
            canvas.fillRect(x - 2, y - 1, w + 4, h + 2, Color::WHITE);
            canvas.setTextColor(Color::BLACK);
            canvas.setCursor(x + 2, y + 2);
            canvas.print(label);
        } else {
            // 非選択: 黒背景に白文字
            canvas.setTextColor(Color::WHITE);
            canvas.setCursor(x + 2, y + 2);
            canvas.print(label);
            // 説明テキスト
            if (desc) {
                canvas.setTextColor(Color::MD_GRAY);
                canvas.setCursor(x + 4, y + 2 + 8);  // ラベルの下に小さく
            }
        }

        manager->transferPartial(x - 2, y - 1, w + 4, h + 2);
    }

    void drawFooter(GFXcanvas16& canvas) {
        canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
        canvas.setTextSize(1);
        canvas.setTextColor(Color::MD_GRAY);
        canvas.setCursor(4, FOOTER_Y + 4);
        canvas.print("CANCEL: BACK");
    }
};
