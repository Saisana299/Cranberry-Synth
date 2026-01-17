#pragma once

#include "ui/ui.hpp"

class HPFScreen : public Screen {
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
        C_CUTOFF,
        C_RESONANCE,
        C_MIX,
        C_BACK,
        C_MAX
    };
    int8_t cursor = C_ENABLED;

    // パラメータ調整の増分（対数スケール）
    const float CUTOFF_STEP_SMALL = 1.05f;  // 5%ずつ
    const float CUTOFF_STEP_LARGE = 1.2f;   // 20%ずつ
    const float RESONANCE_STEP = 0.1f;
    const Gain_t MIX_STEP = 1024; // Q15_MAXの約3%

public:
    HPFScreen() = default;

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
                synth.setHpfEnabled(!synth.isHpfEnabled());
                changed = true;
            }
            else if (cursor == C_CUTOFF) {
                float cutoff = synth.getHpfCutoff();
                bool isLong = (button == BTN_L_LONG);
                cutoff /= isLong ? CUTOFF_STEP_LARGE : CUTOFF_STEP_SMALL;
                if (cutoff < Filter::CUTOFF_MIN) cutoff = Filter::CUTOFF_MIN;
                synth.getFilter().setHighPass(cutoff, synth.getHpfResonance());
                changed = true;
            }
            else if (cursor == C_RESONANCE) {
                float resonance = synth.getHpfResonance() - RESONANCE_STEP;
                if (resonance < Filter::RESONANCE_MIN) resonance = Filter::RESONANCE_MIN;
                synth.getFilter().setHighPass(synth.getHpfCutoff(), resonance);
                changed = true;
            }
            else if (cursor == C_MIX) {
                Gain_t mix = synth.getHpfMix();
                if (mix > MIX_STEP) mix -= MIX_STEP;
                else mix = 0;
                synth.getFilter().setHpfMix(mix);
                changed = true;
            }
        }
        else if (button == BTN_R || button == BTN_R_LONG) {
            if (cursor == C_ENABLED) {
                synth.setHpfEnabled(!synth.isHpfEnabled());
                changed = true;
            }
            else if (cursor == C_CUTOFF) {
                float cutoff = synth.getHpfCutoff();
                bool isLong = (button == BTN_R_LONG);
                cutoff *= isLong ? CUTOFF_STEP_LARGE : CUTOFF_STEP_SMALL;
                if (cutoff > Filter::CUTOFF_MAX) cutoff = Filter::CUTOFF_MAX;
                synth.getFilter().setHighPass(cutoff, synth.getHpfResonance());
                changed = true;
            }
            else if (cursor == C_RESONANCE) {
                float resonance = synth.getHpfResonance() + RESONANCE_STEP;
                if (resonance > Filter::RESONANCE_MAX) resonance = Filter::RESONANCE_MAX;
                synth.getFilter().setHighPass(synth.getHpfCutoff(), resonance);
                changed = true;
            }
            else if (cursor == C_MIX) {
                Gain_t mix = synth.getHpfMix() + MIX_STEP;
                if (mix > Q15_MAX) mix = Q15_MAX;
                synth.getFilter().setHpfMix(mix);
                changed = true;
            }
        }

        // ENTERボタン：トグル or 戻る
        else if (button == BTN_ET) {
            if (cursor == C_ENABLED) {
                synth.setHpfEnabled(!synth.isHpfEnabled());
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
    /**
     * @brief ヘッダー描画
     */
    void drawHeader(GFXcanvas16& canvas) {
        canvas.fillRect(0, 0, SCREEN_WIDTH, HEADER_H, Color::BLACK);
        canvas.setTextSize(1);
        canvas.setTextColor(Color::WHITE);
        canvas.setCursor(2, 2);
        canvas.print("HIGH PASS FILTER");
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
        manager->transferPartial(0, 0, SCREEN_WIDTH, HEADER_H + 1);
    }

    /**
     * @brief すべてのアイテムを描画
     */
    void drawAllItems(GFXcanvas16& canvas) {
        Synth& synth = Synth::getInstance();
        drawToggleItem(canvas, "ENABLED", synth.isHpfEnabled(), 0, cursor == C_ENABLED);
        drawFreqItem(canvas, "CUTOFF", synth.getHpfCutoff(), 1, cursor == C_CUTOFF);
        drawFloatItem(canvas, "Q", synth.getHpfResonance(), 2, cursor == C_RESONANCE);
        drawPercentItem(canvas, "MIX", synth.getHpfMix(), 3, cursor == C_MIX);
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
            drawToggleItem(canvas, "ENABLED", synth.isHpfEnabled(), 0, isSelected);
        }
        else if (cursorPos == C_CUTOFF) {
            drawFreqItem(canvas, "CUTOFF", synth.getHpfCutoff(), 1, isSelected);
        }
        else if (cursorPos == C_RESONANCE) {
            drawFloatItem(canvas, "Q", synth.getHpfResonance(), 2, isSelected);
        }
        else if (cursorPos == C_MIX) {
            drawPercentItem(canvas, "MIX", synth.getHpfMix(), 3, isSelected);
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

        canvas.setCursor(80, y + 4);
        canvas.setTextColor(value ? Color::CYAN : Color::MD_GRAY);
        canvas.print(value ? "ON" : "OFF");

        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    /**
     * @brief 周波数アイテムを描画
     */
    void drawFreqItem(GFXcanvas16& canvas, const char* name, float freq, int index, bool selected) {
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
        char valStr[16];
        if (freq >= 1000.0f) {
            sprintf(valStr, "%.1fkHz", freq / 1000.0f);
        } else {
            sprintf(valStr, "%.0fHz", freq);
        }
        canvas.print(valStr);

        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    /**
     * @brief 浮動小数点アイテムを描画
     */
    void drawFloatItem(GFXcanvas16& canvas, const char* name, float value, int index, bool selected) {
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
        char valStr[16];
        sprintf(valStr, "%.2f", value);
        canvas.print(valStr);

        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    /**
     * @brief パーセントアイテムを描画
     */
    void drawPercentItem(GFXcanvas16& canvas, const char* name, Gain_t value, int index, bool selected) {
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
        char valStr[16];
        sprintf(valStr, "%d%%", (value * 100) / Q15_MAX);
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
