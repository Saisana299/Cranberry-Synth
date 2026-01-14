#pragma once

#include "ui/ui.hpp"
#include "modules/synth.hpp"

class MasterScreen : public Screen {
private:
    const int16_t HEADER_H = 12;
    const int16_t ITEM_H = 16;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 12;

    bool needsFullRedraw = false;

    enum CursorPos {
        C_LEVEL = 0,
        C_PAN,
        C_BACK,
        C_MAX
    };
    int8_t cursor = C_LEVEL;

public:
    MasterScreen() = default;

    void onEnter(UIManager* manager) override {
        this->manager = manager;
        needsFullRedraw = true;
        manager->invalidate();
        manager->triggerFullTransfer();
    }

    bool isAnimated() const override { return false; }

    void handleInput(uint8_t button) override {
        bool moved = false;
        bool changed = false;

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

        // 左右ボタン：値の変更
        else if (button == BTN_L || button == BTN_L_LONG) {
            int8_t dir = -1;
            if (button == BTN_L_LONG) dir = -10;
            adjustParameter(dir);
            changed = true;
        }
        else if (button == BTN_R || button == BTN_R_LONG) {
            int8_t dir = 1;
            if (button == BTN_R_LONG) dir = 10;
            adjustParameter(dir);
            changed = true;
        }

        // ENTERボタン or CANCELボタン：戻る
        else if (button == BTN_ET) {
            if (cursor == C_BACK) {
                manager->popScreen();
                return;
            }
        }
        else if (button == BTN_CXL) {
            manager->popScreen();
            return;
        }

        if (moved || changed) {
            if (changed) needsFullRedraw = true;
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
            updateCursorElement(canvas, lastCursor);
            updateCursorElement(canvas, cursor);
            lastCursor = cursor;
        }
    }

private:
    void drawHeader(GFXcanvas16& canvas) {
        canvas.fillRect(0, 0, SCREEN_WIDTH, HEADER_H, Color::BLACK);
        canvas.setTextSize(1);
        canvas.setTextColor(Color::WHITE);
        canvas.setCursor(2, 2);
        canvas.print("MASTER SETTINGS");
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
        manager->transferPartial(0, 0, SCREEN_WIDTH, HEADER_H + 1);
    }

    void drawAllItems(GFXcanvas16& canvas) {
        Synth& synth = Synth::getInstance();

        // LEVEL (%)
        int16_t level = synth.getMasterLevel();
        char levelStr[8];
        sprintf(levelStr, "%d%%", (level * 100) / 1024);
        drawTextItem(canvas, "LEVEL", levelStr, 0, cursor == C_LEVEL);

        // PAN
        int16_t pan = synth.getMasterPan();
        char panStr[8];
        int16_t displayPan = pan - 100;
        if (displayPan == 0) {
            sprintf(panStr, "C");
        } else if (displayPan < 0) {
            sprintf(panStr, "L%d", -displayPan);
        } else {
            sprintf(panStr, "R%d", displayPan);
        }
        drawTextItem(canvas, "PAN", panStr, 1, cursor == C_PAN);
    }

    void drawFooter(GFXcanvas16& canvas) {
        canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
        drawBackButton(canvas, cursor == C_BACK);
    }

    void updateCursorElement(GFXcanvas16& canvas, int8_t cursorPos) {
        Synth& synth = Synth::getInstance();
        bool isSelected = (cursor == cursorPos);

        if (cursorPos == C_LEVEL) {
            int16_t level = synth.getMasterLevel();
            char levelStr[8];
            sprintf(levelStr, "%d%%", (level * 100) / 1024);
            drawTextItem(canvas, "LEVEL", levelStr, 0, isSelected);
        }
        else if (cursorPos == C_PAN) {
            int16_t pan = synth.getMasterPan();
            char panStr[8];
            int16_t displayPan = pan - 100;
            if (displayPan == 0) {
                sprintf(panStr, "C");
            } else if (displayPan < 0) {
                sprintf(panStr, "L%d", -displayPan);
            } else {
                sprintf(panStr, "R%d", displayPan);
            }
            drawTextItem(canvas, "PAN", panStr, 1, isSelected);
        }
        else if (cursorPos == C_BACK) {
            drawBackButton(canvas, isSelected);
        }
    }

    void drawTextItem(GFXcanvas16& canvas, const char* name, const char* value, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);

        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);

        if (selected) {
            canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(name);

        canvas.setCursor(80, y + 4);
        canvas.setTextColor(Color::WHITE);
        canvas.print(value);

        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    void drawBackButton(GFXcanvas16& canvas, bool selected) {
        int16_t x = 2;
        int16_t y = FOOTER_Y + 2;
        int16_t w = 24;
        int16_t h = 10;

        canvas.fillRect(x, y, w, h, Color::BLACK);

        if (selected) {
            canvas.drawRect(x, y, w, h, Color::WHITE);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(x + 2, y + 1);
        canvas.print("<");

        manager->transferPartial(x, y, w, h);
    }

    void adjustParameter(int8_t direction) {
        Synth& synth = Synth::getInstance();

        switch (cursor) {
            case C_LEVEL: {
                int16_t level = synth.getMasterLevel();
                int16_t step = (direction == 1 || direction == -1) ? 10 : 50;
                int16_t newLevel = level + (direction > 0 ? step : -step);
                synth.setMasterLevel(newLevel);
                break;
            }
            case C_PAN: {
                int16_t pan = synth.getMasterPan();
                int16_t step = (direction == 1 || direction == -1) ? 5 : 20;
                int16_t newPan = pan + (direction > 0 ? step : -step);
                synth.setMasterPan(newPan);
                break;
            }
        }
    }
};
