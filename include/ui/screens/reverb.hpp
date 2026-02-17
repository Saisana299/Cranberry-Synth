#pragma once

#include "ui/ui.hpp"
#include "modules/synth.hpp"

class ReverbScreen : public Screen {
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
        C_ROOM_SIZE,
        C_DAMPING,
        C_MIX,
        C_BACK,
        C_MAX
    };
    int8_t cursor = C_ENABLED;

    // パラメータ調整の増分
    const uint8_t ROOM_STEP = 1;
    const uint8_t DAMP_STEP = 1;
    const Gain_t MIX_STEP = Q15_MAX / 100;  // 1%刻み (約328)

public:
    ReverbScreen() = default;

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

        // 左ボタン：値を減少
        else if (button == BTN_L || button == BTN_L_LONG) {
            if (cursor == C_ENABLED) {
                synth.setReverbEnabled(!synth.isReverbEnabled());
                changed = true;
            }
            else if (cursor == C_ROOM_SIZE) {
                int val = synth.getReverbRoomSize() - ROOM_STEP;
                if (val < REVERB_ROOM_MIN) val = REVERB_ROOM_MIN;
                synth.getReverb().setRoomSize(static_cast<uint8_t>(val));
                changed = true;
            }
            else if (cursor == C_DAMPING) {
                int val = synth.getReverbDamping() - DAMP_STEP;
                if (val < REVERB_DAMP_MIN) val = REVERB_DAMP_MIN;
                synth.getReverb().setDamping(static_cast<uint8_t>(val));
                changed = true;
            }
            else if (cursor == C_MIX) {
                int32_t val = static_cast<int32_t>(synth.getReverbMix()) - MIX_STEP;
                if (val < 0) val = 0;
                synth.getReverb().setMix(static_cast<Gain_t>(val));
                changed = true;
            }
        }

        // 右ボタン：値を増加
        else if (button == BTN_R || button == BTN_R_LONG) {
            if (cursor == C_ENABLED) {
                synth.setReverbEnabled(!synth.isReverbEnabled());
                changed = true;
            }
            else if (cursor == C_ROOM_SIZE) {
                int val = synth.getReverbRoomSize() + ROOM_STEP;
                if (val > REVERB_ROOM_MAX) val = REVERB_ROOM_MAX;
                synth.getReverb().setRoomSize(static_cast<uint8_t>(val));
                changed = true;
            }
            else if (cursor == C_DAMPING) {
                int val = synth.getReverbDamping() + DAMP_STEP;
                if (val > REVERB_DAMP_MAX) val = REVERB_DAMP_MAX;
                synth.getReverb().setDamping(static_cast<uint8_t>(val));
                changed = true;
            }
            else if (cursor == C_MIX) {
                int32_t val = static_cast<int32_t>(synth.getReverbMix()) + MIX_STEP;
                if (val > Q15_MAX) val = Q15_MAX;
                synth.getReverb().setMix(static_cast<Gain_t>(val));
                changed = true;
            }
        }

        // ENTERボタン：トグル or 戻る
        else if (button == BTN_ET) {
            if (cursor == C_ENABLED) {
                synth.setReverbEnabled(!synth.isReverbEnabled());
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
        canvas.print("REVERB");
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
        manager->transferPartial(0, 0, SCREEN_WIDTH, HEADER_H + 1);
    }

    /**
     * @brief すべてのアイテムを描画
     */
    void drawAllItems(GFXcanvas16& canvas) {
        Synth& synth = Synth::getInstance();
        drawToggleItem(canvas, "ENABLED", synth.isReverbEnabled(), 0, cursor == C_ENABLED);
        drawParamItem(canvas, "ROOM", synth.getReverbRoomSize(), "", 1, cursor == C_ROOM_SIZE);
        drawParamItem(canvas, "DAMP", synth.getReverbDamping(), "", 2, cursor == C_DAMPING);
        drawParamItem(canvas, "MIX", (synth.getReverbMix() * 100) / Q15_MAX, "%", 3, cursor == C_MIX);
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
            drawToggleItem(canvas, "ENABLED", synth.isReverbEnabled(), 0, isSelected);
        }
        else if (cursorPos == C_ROOM_SIZE) {
            drawParamItem(canvas, "ROOM", synth.getReverbRoomSize(), "", 1, isSelected);
        }
        else if (cursorPos == C_DAMPING) {
            drawParamItem(canvas, "DAMP", synth.getReverbDamping(), "", 2, isSelected);
        }
        else if (cursorPos == C_MIX) {
            drawParamItem(canvas, "MIX", (synth.getReverbMix() * 100) / Q15_MAX, "%", 3, isSelected);
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
