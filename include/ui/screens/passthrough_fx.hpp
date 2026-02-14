#pragma once

#include "ui/ui.hpp"
#include "modules/passthrough.hpp"

extern Passthrough passthrough;

// ============================================================
// PassthroughLPFScreen — パススルーモード用ローパスフィルター設定
// ============================================================
class PassthroughLPFScreen : public Screen {
private:
    const int16_t HEADER_H = 14;
    const int16_t ITEM_H = 16;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 14;
    bool needsFullRedraw = false;

    enum CursorPos {
        C_ENABLED = 0,
        C_CUTOFF,
        C_RESONANCE,
        C_MIX,
        C_BACK,
        C_MAX
    };
    int8_t cursor = C_ENABLED;

    const float CUTOFF_STEP_SMALL = 1.05f;
    const float CUTOFF_STEP_LARGE = 1.2f;
    const float RESONANCE_STEP = 0.1f;
    const Gain_t MIX_STEP = 1024;

public:
    PassthroughLPFScreen() = default;

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
        Filter& filter = passthrough.getFilter();

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
        else if (button == BTN_L || button == BTN_L_LONG) {
            if (cursor == C_ENABLED) {
                passthrough.setLpfEnabled(!passthrough.isLpfEnabled());
                changed = true;
            }
            else if (cursor == C_CUTOFF) {
                float cutoff = filter.getLpfCutoff();
                cutoff /= (button == BTN_L_LONG) ? CUTOFF_STEP_LARGE : CUTOFF_STEP_SMALL;
                if (cutoff < Filter::CUTOFF_MIN) cutoff = Filter::CUTOFF_MIN;
                filter.setLowPass(cutoff, filter.getLpfResonance());
                changed = true;
            }
            else if (cursor == C_RESONANCE) {
                float resonance = filter.getLpfResonance() - RESONANCE_STEP;
                if (resonance < Filter::RESONANCE_MIN) resonance = Filter::RESONANCE_MIN;
                filter.setLowPass(filter.getLpfCutoff(), resonance);
                changed = true;
            }
            else if (cursor == C_MIX) {
                Gain_t mix = filter.getLpfMix();
                if (mix > MIX_STEP) mix -= MIX_STEP;
                else mix = 0;
                filter.setLpfMix(mix);
                changed = true;
            }
        }
        else if (button == BTN_R || button == BTN_R_LONG) {
            if (cursor == C_ENABLED) {
                passthrough.setLpfEnabled(!passthrough.isLpfEnabled());
                changed = true;
            }
            else if (cursor == C_CUTOFF) {
                float cutoff = filter.getLpfCutoff();
                cutoff *= (button == BTN_R_LONG) ? CUTOFF_STEP_LARGE : CUTOFF_STEP_SMALL;
                if (cutoff > Filter::CUTOFF_MAX) cutoff = Filter::CUTOFF_MAX;
                filter.setLowPass(cutoff, filter.getLpfResonance());
                changed = true;
            }
            else if (cursor == C_RESONANCE) {
                float resonance = filter.getLpfResonance() + RESONANCE_STEP;
                if (resonance > Filter::RESONANCE_MAX) resonance = Filter::RESONANCE_MAX;
                filter.setLowPass(filter.getLpfCutoff(), resonance);
                changed = true;
            }
            else if (cursor == C_MIX) {
                int32_t mix = filter.getLpfMix() + MIX_STEP;
                if (mix > Q15_MAX) mix = Q15_MAX;
                filter.setLpfMix(static_cast<Gain_t>(mix));
                changed = true;
            }
        }
        else if (button == BTN_ET) {
            if (cursor == C_ENABLED) {
                passthrough.setLpfEnabled(!passthrough.isLpfEnabled());
                changed = true;
            }
            else if (cursor == C_BACK) {
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

        if (firstDraw) {
            canvas.fillScreen(Color::BLACK);
            drawHeader(canvas);
            drawAllItems(canvas);
            drawFooter(canvas);
            firstDraw = false;
            lastCursor = cursor;
            manager->triggerFullTransfer();
        }

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
        canvas.print("LOW PASS FILTER");
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
        manager->transferPartial(0, 0, SCREEN_WIDTH, HEADER_H + 1);
    }

    void drawAllItems(GFXcanvas16& canvas) {
        Filter& filter = passthrough.getFilter();
        drawToggleItem(canvas, "ENABLED", passthrough.isLpfEnabled(), 0, cursor == C_ENABLED);
        drawFreqItem(canvas, "CUTOFF", filter.getLpfCutoff(), 1, cursor == C_CUTOFF);
        drawFloatItem(canvas, "Q", filter.getLpfResonance(), 2, cursor == C_RESONANCE);
        drawPercentItem(canvas, "MIX", filter.getLpfMix(), 3, cursor == C_MIX);
    }

    void drawFooter(GFXcanvas16& canvas) {
        canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
        drawBackButton(canvas, cursor == C_BACK);
    }

    void updateCursorElement(GFXcanvas16& canvas, int8_t cursorPos) {
        Filter& filter = passthrough.getFilter();
        bool isSelected = (cursor == cursorPos);

        if (cursorPos == C_ENABLED) drawToggleItem(canvas, "ENABLED", passthrough.isLpfEnabled(), 0, isSelected);
        else if (cursorPos == C_CUTOFF) drawFreqItem(canvas, "CUTOFF", filter.getLpfCutoff(), 1, isSelected);
        else if (cursorPos == C_RESONANCE) drawFloatItem(canvas, "Q", filter.getLpfResonance(), 2, isSelected);
        else if (cursorPos == C_MIX) drawPercentItem(canvas, "MIX", filter.getLpfMix(), 3, isSelected);
        else if (cursorPos == C_BACK) drawBackButton(canvas, isSelected);
    }

    void drawToggleItem(GFXcanvas16& canvas, const char* name, bool value, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);
        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);
        if (selected) canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(name);
        canvas.setCursor(80, y + 4);
        canvas.setTextColor(value ? Color::CYAN : Color::MD_GRAY);
        canvas.print(value ? "ON" : "OFF");
        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    void drawFreqItem(GFXcanvas16& canvas, const char* name, float freq, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);
        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);
        if (selected) canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(name);
        canvas.setCursor(80, y + 4);
        canvas.setTextColor(Color::WHITE);
        char valStr[16];
        if (freq >= 1000.0f) sprintf(valStr, "%.1fkHz", freq / 1000.0f);
        else sprintf(valStr, "%.0fHz", freq);
        canvas.print(valStr);
        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    void drawFloatItem(GFXcanvas16& canvas, const char* name, float value, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);
        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);
        if (selected) canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
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

    void drawPercentItem(GFXcanvas16& canvas, const char* name, Gain_t value, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);
        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);
        if (selected) canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(name);
        canvas.setCursor(80, y + 4);
        canvas.setTextColor(Color::WHITE);
        char valStr[16];
        sprintf(valStr, "%d%%", (int)((int32_t)value * 100 / Q15_MAX));
        canvas.print(valStr);
        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    void drawBackButton(GFXcanvas16& canvas, bool selected) {
        int16_t x = 2;
        int16_t y = FOOTER_Y + 2;
        int16_t w = 24;
        int16_t h = 10;
        canvas.fillRect(x, y, w, h, Color::BLACK);
        if (selected) canvas.drawRect(x, y, w, h, Color::WHITE);
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(x + 2, y + 1);
        canvas.print("<");
        manager->transferPartial(x, y, w, h);
    }
};

// ============================================================
// PassthroughHPFScreen — パススルーモード用ハイパスフィルター設定
// ============================================================
class PassthroughHPFScreen : public Screen {
private:
    const int16_t HEADER_H = 14;
    const int16_t ITEM_H = 16;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 14;
    bool needsFullRedraw = false;

    enum CursorPos {
        C_ENABLED = 0,
        C_CUTOFF,
        C_RESONANCE,
        C_MIX,
        C_BACK,
        C_MAX
    };
    int8_t cursor = C_ENABLED;

    const float CUTOFF_STEP_SMALL = 1.05f;
    const float CUTOFF_STEP_LARGE = 1.2f;
    const float RESONANCE_STEP = 0.1f;
    const Gain_t MIX_STEP = 1024;

public:
    PassthroughHPFScreen() = default;

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
        Filter& filter = passthrough.getFilter();

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
        else if (button == BTN_L || button == BTN_L_LONG) {
            if (cursor == C_ENABLED) {
                passthrough.setHpfEnabled(!passthrough.isHpfEnabled());
                changed = true;
            }
            else if (cursor == C_CUTOFF) {
                float cutoff = filter.getHpfCutoff();
                cutoff /= (button == BTN_L_LONG) ? CUTOFF_STEP_LARGE : CUTOFF_STEP_SMALL;
                if (cutoff < Filter::HPF_CUTOFF_MIN) cutoff = Filter::HPF_CUTOFF_MIN;
                filter.setHighPass(cutoff, filter.getHpfResonance());
                changed = true;
            }
            else if (cursor == C_RESONANCE) {
                float resonance = filter.getHpfResonance() - RESONANCE_STEP;
                if (resonance < Filter::RESONANCE_MIN) resonance = Filter::RESONANCE_MIN;
                filter.setHighPass(filter.getHpfCutoff(), resonance);
                changed = true;
            }
            else if (cursor == C_MIX) {
                Gain_t mix = filter.getHpfMix();
                if (mix > MIX_STEP) mix -= MIX_STEP;
                else mix = 0;
                filter.setHpfMix(mix);
                changed = true;
            }
        }
        else if (button == BTN_R || button == BTN_R_LONG) {
            if (cursor == C_ENABLED) {
                passthrough.setHpfEnabled(!passthrough.isHpfEnabled());
                changed = true;
            }
            else if (cursor == C_CUTOFF) {
                float cutoff = filter.getHpfCutoff();
                cutoff *= (button == BTN_R_LONG) ? CUTOFF_STEP_LARGE : CUTOFF_STEP_SMALL;
                if (cutoff > Filter::CUTOFF_MAX) cutoff = Filter::CUTOFF_MAX;
                filter.setHighPass(cutoff, filter.getHpfResonance());
                changed = true;
            }
            else if (cursor == C_RESONANCE) {
                float resonance = filter.getHpfResonance() + RESONANCE_STEP;
                if (resonance > Filter::RESONANCE_MAX) resonance = Filter::RESONANCE_MAX;
                filter.setHighPass(filter.getHpfCutoff(), resonance);
                changed = true;
            }
            else if (cursor == C_MIX) {
                int32_t mix = filter.getHpfMix() + MIX_STEP;
                if (mix > Q15_MAX) mix = Q15_MAX;
                filter.setHpfMix(static_cast<Gain_t>(mix));
                changed = true;
            }
        }
        else if (button == BTN_ET) {
            if (cursor == C_ENABLED) {
                passthrough.setHpfEnabled(!passthrough.isHpfEnabled());
                changed = true;
            }
            else if (cursor == C_BACK) {
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

        if (firstDraw) {
            canvas.fillScreen(Color::BLACK);
            drawHeader(canvas);
            drawAllItems(canvas);
            drawFooter(canvas);
            firstDraw = false;
            lastCursor = cursor;
            manager->triggerFullTransfer();
        }

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
        canvas.print("HIGH PASS FILTER");
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
        manager->transferPartial(0, 0, SCREEN_WIDTH, HEADER_H + 1);
    }

    void drawAllItems(GFXcanvas16& canvas) {
        Filter& filter = passthrough.getFilter();
        drawToggleItem(canvas, "ENABLED", passthrough.isHpfEnabled(), 0, cursor == C_ENABLED);
        drawFreqItem(canvas, "CUTOFF", filter.getHpfCutoff(), 1, cursor == C_CUTOFF);
        drawFloatItem(canvas, "Q", filter.getHpfResonance(), 2, cursor == C_RESONANCE);
        drawPercentItem(canvas, "MIX", filter.getHpfMix(), 3, cursor == C_MIX);
    }

    void drawFooter(GFXcanvas16& canvas) {
        canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
        drawBackButton(canvas, cursor == C_BACK);
    }

    void updateCursorElement(GFXcanvas16& canvas, int8_t cursorPos) {
        Filter& filter = passthrough.getFilter();
        bool isSelected = (cursor == cursorPos);

        if (cursorPos == C_ENABLED) drawToggleItem(canvas, "ENABLED", passthrough.isHpfEnabled(), 0, isSelected);
        else if (cursorPos == C_CUTOFF) drawFreqItem(canvas, "CUTOFF", filter.getHpfCutoff(), 1, isSelected);
        else if (cursorPos == C_RESONANCE) drawFloatItem(canvas, "Q", filter.getHpfResonance(), 2, isSelected);
        else if (cursorPos == C_MIX) drawPercentItem(canvas, "MIX", filter.getHpfMix(), 3, isSelected);
        else if (cursorPos == C_BACK) drawBackButton(canvas, isSelected);
    }

    void drawToggleItem(GFXcanvas16& canvas, const char* name, bool value, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);
        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);
        if (selected) canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(name);
        canvas.setCursor(80, y + 4);
        canvas.setTextColor(value ? Color::CYAN : Color::MD_GRAY);
        canvas.print(value ? "ON" : "OFF");
        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    void drawFreqItem(GFXcanvas16& canvas, const char* name, float freq, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);
        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);
        if (selected) canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(name);
        canvas.setCursor(80, y + 4);
        canvas.setTextColor(Color::WHITE);
        char valStr[16];
        if (freq >= 1000.0f) sprintf(valStr, "%.1fkHz", freq / 1000.0f);
        else sprintf(valStr, "%.0fHz", freq);
        canvas.print(valStr);
        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    void drawFloatItem(GFXcanvas16& canvas, const char* name, float value, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);
        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);
        if (selected) canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
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

    void drawPercentItem(GFXcanvas16& canvas, const char* name, Gain_t value, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);
        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);
        if (selected) canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(name);
        canvas.setCursor(80, y + 4);
        canvas.setTextColor(Color::WHITE);
        char valStr[16];
        sprintf(valStr, "%d%%", (int)((int32_t)value * 100 / Q15_MAX));
        canvas.print(valStr);
        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    void drawBackButton(GFXcanvas16& canvas, bool selected) {
        int16_t x = 2;
        int16_t y = FOOTER_Y + 2;
        int16_t w = 24;
        int16_t h = 10;
        canvas.fillRect(x, y, w, h, Color::BLACK);
        if (selected) canvas.drawRect(x, y, w, h, Color::WHITE);
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(x + 2, y + 1);
        canvas.print("<");
        manager->transferPartial(x, y, w, h);
    }
};

// ============================================================
// PassthroughDelayScreen — パススルーモード用ディレイ設定
// ============================================================
class PassthroughDelayScreen : public Screen {
private:
    const int16_t HEADER_H = 14;
    const int16_t ITEM_H = 16;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 14;
    bool needsFullRedraw = false;

    enum CursorPos {
        C_ENABLED = 0,
        C_TIME,
        C_LEVEL,
        C_FEEDBACK,
        C_BACK,
        C_MAX
    };
    int8_t cursor = C_ENABLED;

    const int32_t TIME_STEP = 5;
    const Gain_t LEVEL_STEP = Q15_MAX / 100;
    const Gain_t FEEDBACK_STEP = Q15_MAX / 100;

public:
    PassthroughDelayScreen() = default;

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
        Delay& delay = passthrough.getDelay();

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
        else if (button == BTN_L || button == BTN_L_LONG) {
            if (cursor == C_ENABLED) {
                passthrough.setDelayEnabled(!passthrough.isDelayEnabled());
                changed = true;
            }
            else if (cursor == C_TIME) {
                int32_t time = delay.getTime() - TIME_STEP;
                if (time < MIN_TIME) time = MIN_TIME;
                delay.setTime(time);
                changed = true;
            }
            else if (cursor == C_LEVEL) {
                int32_t level = static_cast<int32_t>(delay.getLevel()) - LEVEL_STEP;
                if (level < MIN_LEVEL) level = MIN_LEVEL;
                delay.setLevel(static_cast<Gain_t>(level));
                changed = true;
            }
            else if (cursor == C_FEEDBACK) {
                int32_t feedback = static_cast<int32_t>(delay.getFeedback()) - FEEDBACK_STEP;
                if (feedback < MIN_FEEDBACK) feedback = MIN_FEEDBACK;
                delay.setFeedback(static_cast<Gain_t>(feedback));
                changed = true;
            }
        }
        else if (button == BTN_R || button == BTN_R_LONG) {
            if (cursor == C_ENABLED) {
                passthrough.setDelayEnabled(!passthrough.isDelayEnabled());
                changed = true;
            }
            else if (cursor == C_TIME) {
                int32_t time = delay.getTime() + TIME_STEP;
                if (time > MAX_TIME) time = MAX_TIME;
                delay.setTime(time);
                changed = true;
            }
            else if (cursor == C_LEVEL) {
                int32_t level = static_cast<int32_t>(delay.getLevel()) + LEVEL_STEP;
                if (level > MAX_LEVEL) level = MAX_LEVEL;
                delay.setLevel(static_cast<Gain_t>(level));
                changed = true;
            }
            else if (cursor == C_FEEDBACK) {
                int32_t feedback = static_cast<int32_t>(delay.getFeedback()) + FEEDBACK_STEP;
                if (feedback > MAX_FEEDBACK) feedback = MAX_FEEDBACK;
                delay.setFeedback(static_cast<Gain_t>(feedback));
                changed = true;
            }
        }
        else if (button == BTN_ET) {
            if (cursor == C_ENABLED) {
                passthrough.setDelayEnabled(!passthrough.isDelayEnabled());
                changed = true;
            }
            else if (cursor == C_BACK) {
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

        if (firstDraw) {
            canvas.fillScreen(Color::BLACK);
            drawHeader(canvas);
            drawAllItems(canvas);
            drawFooter(canvas);
            firstDraw = false;
            lastCursor = cursor;
            manager->triggerFullTransfer();
        }

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
        canvas.print("DELAY");
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
        manager->transferPartial(0, 0, SCREEN_WIDTH, HEADER_H + 1);
    }

    void drawAllItems(GFXcanvas16& canvas) {
        Delay& delay = passthrough.getDelay();
        drawToggleItem(canvas, "ENABLED", passthrough.isDelayEnabled(), 0, cursor == C_ENABLED);
        drawParamItem(canvas, "TIME", delay.getTime(), "ms", 1, cursor == C_TIME);
        drawParamItem(canvas, "LEVEL", (delay.getLevel() * 100) / Q15_MAX, "%", 2, cursor == C_LEVEL);
        drawParamItem(canvas, "FEEDBACK", (delay.getFeedback() * 100) / Q15_MAX, "%", 3, cursor == C_FEEDBACK);
    }

    void drawFooter(GFXcanvas16& canvas) {
        canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
        drawBackButton(canvas, cursor == C_BACK);
    }

    void updateCursorElement(GFXcanvas16& canvas, int8_t cursorPos) {
        Delay& delay = passthrough.getDelay();
        bool isSelected = (cursor == cursorPos);

        if (cursorPos == C_ENABLED) drawToggleItem(canvas, "ENABLED", passthrough.isDelayEnabled(), 0, isSelected);
        else if (cursorPos == C_TIME) drawParamItem(canvas, "TIME", delay.getTime(), "ms", 1, isSelected);
        else if (cursorPos == C_LEVEL) drawParamItem(canvas, "LEVEL", (delay.getLevel() * 100) / Q15_MAX, "%", 2, isSelected);
        else if (cursorPos == C_FEEDBACK) drawParamItem(canvas, "FEEDBACK", (delay.getFeedback() * 100) / Q15_MAX, "%", 3, isSelected);
        else if (cursorPos == C_BACK) drawBackButton(canvas, isSelected);
    }

    void drawToggleItem(GFXcanvas16& canvas, const char* name, bool value, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);
        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);
        if (selected) canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(name);
        canvas.setCursor(80, y + 4);
        canvas.setTextColor(value ? Color::CYAN : Color::MD_GRAY);
        canvas.print(value ? "ON" : "OFF");
        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    void drawParamItem(GFXcanvas16& canvas, const char* name, int32_t value, const char* unit, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);
        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);
        if (selected) canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(name);
        canvas.setCursor(80, y + 4);
        canvas.setTextColor(Color::WHITE);
        char valStr[16];
        sprintf(valStr, "%ld%s", value, unit);
        canvas.print(valStr);
        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }

    void drawBackButton(GFXcanvas16& canvas, bool selected) {
        int16_t x = 2;
        int16_t y = FOOTER_Y + 2;
        int16_t w = 24;
        int16_t h = 10;
        canvas.fillRect(x, y, w, h, Color::BLACK);
        if (selected) canvas.drawRect(x, y, w, h, Color::WHITE);
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(x + 2, y + 1);
        canvas.print("<");
        manager->transferPartial(x, y, w, h);
    }
};
