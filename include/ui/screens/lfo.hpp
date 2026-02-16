#pragma once

#include "ui/ui.hpp"
#include "modules/synth.hpp"

/**
 * @brief LFOパラメータ編集画面
 *
 * MasterScreen から遷移。8パラメータ + BACKボタン。
 * 128x128 OLED に収めるため ITEM_H=12px。
 */
class LFOScreen : public Screen {
private:
    const int16_t HEADER_H = 12;
    const int16_t ITEM_H = 12;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 12;

    bool needsFullRedraw = false;

    enum CursorPos {
        C_WAVE = 0,
        C_SPEED,
        C_DELAY,
        C_PM_DEPTH,
        C_AM_DEPTH,
        C_PM_SENS,
        C_KEY_SYNC,
        C_OSC_KEY_SYNC,
        C_BACK,
        C_MAX
    };
    int8_t cursor = C_WAVE;

    // スクロール管理: 画面に表示できるアイテム数
    static constexpr int8_t VISIBLE_ITEMS = 8;
    int8_t scrollOffset = 0;

    void updateScroll() {
        // C_BACK はフッター固定なのでスクロール対象は C_WAVE～C_OSC_KEY_SYNC (8項目)
        if (cursor == C_BACK) return;
        if (cursor < scrollOffset) {
            scrollOffset = cursor;
        }
        else if (cursor >= scrollOffset + VISIBLE_ITEMS) {
            scrollOffset = cursor - VISIBLE_ITEMS + 1;
        }
    }

public:
    LFOScreen() = default;

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

        // ENTERボタン：トグル or 戻る
        else if (button == BTN_ET) {
            if (cursor == C_KEY_SYNC) {
                synth.getLfo().setKeySync(!synth.getLfo().getKeySync());
                changed = true;
            }
            else if (cursor == C_OSC_KEY_SYNC) {
                synth.setOscKeySync(!synth.getOscKeySync());
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
            int8_t oldScroll = scrollOffset;
            updateScroll();
            if (changed || scrollOffset != oldScroll) needsFullRedraw = true;
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
        canvas.print("LFO");
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
        manager->transferPartial(0, 0, SCREEN_WIDTH, HEADER_H + 1);
    }

    void drawAllItems(GFXcanvas16& canvas) {
        Synth& synth = Synth::getInstance();
        Lfo& lfo = synth.getLfo();

        // WAVE
        drawTextItem(canvas, "WAVE", Lfo::getWaveName(lfo.getWave()), C_WAVE, cursor == C_WAVE);
        // SPEED
        drawNumItem(canvas, "SPEED", lfo.getSpeed(), C_SPEED, cursor == C_SPEED);
        // DELAY
        drawNumItem(canvas, "DELAY", lfo.getDelay(), C_DELAY, cursor == C_DELAY);
        // PM DEPTH
        drawNumItem(canvas, "PM DEPTH", lfo.getPmDepth(), C_PM_DEPTH, cursor == C_PM_DEPTH);
        // AM DEPTH
        drawNumItem(canvas, "AM DEPTH", lfo.getAmDepth(), C_AM_DEPTH, cursor == C_AM_DEPTH);
        // P.M.SENS
        drawNumItem(canvas, "P.M.SENS", lfo.getPitchModSens(), C_PM_SENS, cursor == C_PM_SENS);
        // KEY SYNC
        drawToggleItem(canvas, "KEY SYNC", lfo.getKeySync(), C_KEY_SYNC, cursor == C_KEY_SYNC);
        // OSC KEY SYNC
        drawToggleItem(canvas, "OSC SYNC", synth.getOscKeySync(), C_OSC_KEY_SYNC, cursor == C_OSC_KEY_SYNC);
    }

    void drawFooter(GFXcanvas16& canvas) {
        canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
        drawBackButton(canvas, cursor == C_BACK);
    }

    void updateCursorElement(GFXcanvas16& canvas, int8_t pos) {
        Synth& synth = Synth::getInstance();
        Lfo& lfo = synth.getLfo();
        bool sel = (cursor == pos);

        switch (pos) {
            case C_WAVE:         drawTextItem(canvas, "WAVE", Lfo::getWaveName(lfo.getWave()), pos, sel); break;
            case C_SPEED:        drawNumItem(canvas, "SPEED", lfo.getSpeed(), pos, sel); break;
            case C_DELAY:        drawNumItem(canvas, "DELAY", lfo.getDelay(), pos, sel); break;
            case C_PM_DEPTH:     drawNumItem(canvas, "PM DEPTH", lfo.getPmDepth(), pos, sel); break;
            case C_AM_DEPTH:     drawNumItem(canvas, "AM DEPTH", lfo.getAmDepth(), pos, sel); break;
            case C_PM_SENS:      drawNumItem(canvas, "P.M.SENS", lfo.getPitchModSens(), pos, sel); break;
            case C_KEY_SYNC:     drawToggleItem(canvas, "KEY SYNC", lfo.getKeySync(), pos, sel); break;
            case C_OSC_KEY_SYNC: drawToggleItem(canvas, "OSC SYNC", synth.getOscKeySync(), pos, sel); break;
            case C_BACK:         drawBackButton(canvas, sel); break;
        }
    }

    /**
     * @brief アイテムのY座標を計算（スクロール対応）
     * @return Y座標。表示範囲外なら -1
     */
    int16_t itemY(int8_t pos) const {
        int8_t visibleIdx = pos - scrollOffset;
        if (visibleIdx < 0 || visibleIdx >= VISIBLE_ITEMS) return -1;
        return HEADER_H + 1 + (visibleIdx * ITEM_H);
    }

    void drawTextItem(GFXcanvas16& canvas, const char* name, const char* value, int8_t pos, bool selected) {
        int16_t y = itemY(pos);
        if (y < 0) return;

        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);

        if (selected) {
            canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(8, y + 3);
        canvas.print(name);

        canvas.setCursor(90, y + 3);
        canvas.setTextColor(Color::WHITE);
        canvas.print(value);

        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    void drawNumItem(GFXcanvas16& canvas, const char* name, int32_t value, int8_t pos, bool selected) {
        int16_t y = itemY(pos);
        if (y < 0) return;

        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);

        if (selected) {
            canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(8, y + 3);
        canvas.print(name);

        char valStr[8];
        sprintf(valStr, "%ld", value);
        canvas.setCursor(90, y + 3);
        canvas.setTextColor(Color::WHITE);
        canvas.print(valStr);

        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    void drawToggleItem(GFXcanvas16& canvas, const char* name, bool value, int8_t pos, bool selected) {
        int16_t y = itemY(pos);
        if (y < 0) return;

        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);

        if (selected) {
            canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(8, y + 3);
        canvas.print(name);

        canvas.setCursor(90, y + 3);
        canvas.setTextColor(value ? Color::CYAN : Color::MD_GRAY);
        canvas.print(value ? "ON" : "OFF");

        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    void drawBackButton(GFXcanvas16& canvas, bool selected) {
        int16_t x = 2;
        int16_t y = FOOTER_Y + 1;
        int16_t w = 24;
        int16_t h = 10;

        canvas.fillRect(x, y, w, h, Color::BLACK);

        if (selected) {
            canvas.drawRect(x, y, w, h, Color::WHITE);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(x + 2, y + 2);
        canvas.print("<");

        manager->transferPartial(x, y, w, h);
    }

    void adjustParameter(int8_t direction) {
        Synth& synth = Synth::getInstance();
        Lfo& lfo = synth.getLfo();

        // 長押し（|dir|>=10）の場合は大きいステップ、短押しは1
        int8_t step1 = (direction > 0) ? 1 : -1;
        int8_t stepN = direction;

        switch (cursor) {
            case C_WAVE: {
                int8_t w = static_cast<int8_t>(lfo.getWave()) + step1;
                if (w < 0) w = Lfo::WAVE_COUNT - 1;
                if (w >= Lfo::WAVE_COUNT) w = 0;
                lfo.setWave(w);
                break;
            }
            case C_SPEED: {
                int16_t v = lfo.getSpeed() + stepN;
                if (v < 0) v = 0;
                if (v > 99) v = 99;
                lfo.setSpeed(v);
                break;
            }
            case C_DELAY: {
                int16_t v = lfo.getDelay() + stepN;
                if (v < 0) v = 0;
                if (v > 99) v = 99;
                lfo.setDelay(v);
                break;
            }
            case C_PM_DEPTH: {
                int16_t v = lfo.getPmDepth() + stepN;
                if (v < 0) v = 0;
                if (v > 99) v = 99;
                lfo.setPmDepth(v);
                break;
            }
            case C_AM_DEPTH: {
                int16_t v = lfo.getAmDepth() + stepN;
                if (v < 0) v = 0;
                if (v > 99) v = 99;
                lfo.setAmDepth(v);
                break;
            }
            case C_PM_SENS: {
                int8_t v = static_cast<int8_t>(lfo.getPitchModSens()) + step1;
                if (v < 0) v = 0;
                if (v > 7) v = 7;
                lfo.setPitchModSens(v);
                break;
            }
            case C_KEY_SYNC: {
                lfo.setKeySync(!lfo.getKeySync());
                break;
            }
            case C_OSC_KEY_SYNC: {
                synth.setOscKeySync(!synth.getOscKeySync());
                break;
            }
        }
    }
};
