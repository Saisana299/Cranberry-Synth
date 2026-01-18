#pragma once

#include "ui/ui.hpp"
#include "modules/synth.hpp"

class DelayScreen : public Screen {
private:
    // 定数
    const int16_t HEADER_H = 14;
    const int16_t ITEM_H = 16;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 14;

    // 状態変数
    bool needsFullRedraw = false;

    // --- カーソル管理 ---
    enum CursorPos {
        C_ENABLED = 0,
        C_TIME,
        C_LEVEL,
        C_FEEDBACK,
        C_BACK,
        C_MAX
    };
    int8_t cursor = C_ENABLED;

    // パラメータ調整の増分
    const int32_t TIME_STEP = 5;                      // 5ms
    const Gain_t LEVEL_STEP = Q15_MAX / 100;          // 1%刻み (約328)
    const Gain_t FEEDBACK_STEP = Q15_MAX / 100;       // 1%刻み (約328)

public:
    DelayScreen() = default;

    void onEnter(UIManager* manager) override {
        this->manager = manager;
        cursor = C_ENABLED;
        needsFullRedraw = true;
        manager->invalidate();
        manager->triggerFullTransfer();
    }

    bool isAnimated() const override { return false; }

    void handleInput(uint8_t button) override {
        bool moved = false;
        bool changed = false;
        Synth& synth = Synth::getInstance();

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
            if (cursor == C_ENABLED) {
                synth.setDelayEnabled(!synth.isDelayEnabled());
                changed = true;
            }
            else if (cursor == C_TIME) {
                int32_t time = synth.getDelayTime() - TIME_STEP;
                if (time < MIN_TIME) time = MIN_TIME;
                synth.getDelay().setTime(time);
                changed = true;
            }
            else if (cursor == C_LEVEL) {
                int32_t level = static_cast<int32_t>(synth.getDelayLevel()) - LEVEL_STEP;
                if (level < MIN_LEVEL) level = MIN_LEVEL;
                synth.getDelay().setLevel(static_cast<Gain_t>(level));
                changed = true;
            }
            else if (cursor == C_FEEDBACK) {
                int32_t feedback = static_cast<int32_t>(synth.getDelayFeedback()) - FEEDBACK_STEP;
                if (feedback < MIN_FEEDBACK) feedback = MIN_FEEDBACK;
                synth.getDelay().setFeedback(static_cast<Gain_t>(feedback));
                changed = true;
            }
        }
        else if (button == BTN_R || button == BTN_R_LONG) {
            if (cursor == C_ENABLED) {
                synth.setDelayEnabled(!synth.isDelayEnabled());
                changed = true;
            }
            else if (cursor == C_TIME) {
                int32_t time = synth.getDelayTime() + TIME_STEP;
                if (time > MAX_TIME) time = MAX_TIME;
                synth.getDelay().setTime(time);
                changed = true;
            }
            else if (cursor == C_LEVEL) {
                int32_t level = static_cast<int32_t>(synth.getDelayLevel()) + LEVEL_STEP;
                if (level > MAX_LEVEL) level = MAX_LEVEL;
                synth.getDelay().setLevel(static_cast<Gain_t>(level));
                changed = true;
            }
            else if (cursor == C_FEEDBACK) {
                int32_t feedback = static_cast<int32_t>(synth.getDelayFeedback()) + FEEDBACK_STEP;
                if (feedback > MAX_FEEDBACK) feedback = MAX_FEEDBACK;
                synth.getDelay().setFeedback(static_cast<Gain_t>(feedback));
                changed = true;
            }
        }

        // ENTERボタン：トグル or 戻る
        else if (button == BTN_ET) {
            if (cursor == C_ENABLED) {
                synth.setDelayEnabled(!synth.isDelayEnabled());
                changed = true;
            }
            else if (cursor == C_BACK) {
                manager->popScreen();
                return;
            }
        }

        // CANCELボタン：戻る
        else if (button == BTN_CXL) {
            manager->popScreen();
            return;
        }

        if (moved || changed) {
            if (changed) needsFullRedraw = true; // パラメータ変更時は全体再描画
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
    /**
     * @brief ヘッダー描画
     */
    void drawHeader(GFXcanvas16& canvas) {
        canvas.fillRect(0, 0, SCREEN_WIDTH, HEADER_H, Color::BLACK);
        canvas.setTextSize(1);
        canvas.setTextColor(Color::WHITE);
        canvas.setCursor(2, 2);
        canvas.print("DELAY");
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
        manager->transferPartial(0, 0, SCREEN_WIDTH, HEADER_H + 1);
    }

    /**
     * @brief すべてのアイテムを描画
     */
    void drawAllItems(GFXcanvas16& canvas) {
        Synth& synth = Synth::getInstance();
        drawToggleItem(canvas, "ENABLED", synth.isDelayEnabled(), 0, cursor == C_ENABLED);
        drawParamItem(canvas, "TIME", synth.getDelayTime(), "ms", 1, cursor == C_TIME);
        drawParamItem(canvas, "LEVEL", (synth.getDelayLevel() * 100) / Q15_MAX, "%", 2, cursor == C_LEVEL);
        drawParamItem(canvas, "FEEDBACK", (synth.getDelayFeedback() * 100) / Q15_MAX, "%", 3, cursor == C_FEEDBACK);
    }

    /**
     * @brief フッター（BACKボタン）を描画
     */
    void drawFooter(GFXcanvas16& canvas) {
        canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
        drawBackButton(canvas, cursor == C_BACK);
    }

    /**
     * @brief カーソル位置の要素を更新
     */
    void updateCursorElement(GFXcanvas16& canvas, int8_t cursorPos) {
        Synth& synth = Synth::getInstance();
        bool isSelected = (cursor == cursorPos);

        if (cursorPos == C_ENABLED) {
            drawToggleItem(canvas, "ENABLED", synth.isDelayEnabled(), 0, isSelected);
        }
        else if (cursorPos == C_TIME) {
            drawParamItem(canvas, "TIME", synth.getDelayTime(), "ms", 1, isSelected);
        }
        else if (cursorPos == C_LEVEL) {
            drawParamItem(canvas, "LEVEL", (synth.getDelayLevel() * 100) / Q15_MAX, "%", 2, isSelected);
        }
        else if (cursorPos == C_FEEDBACK) {
            drawParamItem(canvas, "FEEDBACK", (synth.getDelayFeedback() * 100) / Q15_MAX, "%", 3, isSelected);
        }
        else if (cursorPos == C_BACK) {
            drawBackButton(canvas, isSelected);
        }
    }

    /**
     * @brief トグルアイテムを描画
     */
    void drawToggleItem(GFXcanvas16& canvas, const char* name, bool value, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);

        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);

        if (selected) {
            canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(name);

        // 値を表示（右側）
        canvas.setCursor(80, y + 4);
        canvas.setTextColor(value ? Color::CYAN : Color::MD_GRAY);
        canvas.print(value ? "ON" : "OFF");

        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    /**
     * @brief パラメータアイテムを描画
     */
    void drawParamItem(GFXcanvas16& canvas, const char* name, int32_t value, const char* unit, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);

        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);

        if (selected) {
            canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(name);

        // 値を表示（右側）
        canvas.setCursor(80, y + 4);
        canvas.setTextColor(Color::WHITE);
        char valStr[16];
        sprintf(valStr, "%ld%s", value, unit);
        canvas.print(valStr);

        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    /**
     * @brief BACKボタンを描画
     */
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
};
