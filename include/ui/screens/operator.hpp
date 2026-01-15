#pragma once

#include "ui/ui.hpp"

// ===== エンベロープ編集画面 (Rate/Level) =====
class OperatorEnvelopeScreen : public Screen {
private:
    const int16_t HEADER_H = 12;
    const int16_t ITEM_H = 10;   // コンパクトに
    const int16_t GRAPH_Y = 60;  // グラフ開始位置（R4/L4から6px空け）
    const int16_t GRAPH_H = 52;  // グラフ高さ（大きめに）
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 12;

    bool needsFullRedraw = false;
    uint8_t operatorIndex = 0;

    enum CursorPos {
        C_RATE1 = 0,
        C_RATE2,
        C_RATE3,
        C_RATE4,
        C_LEVEL1,
        C_LEVEL2,
        C_LEVEL3,
        C_LEVEL4,
        C_BACK,
        C_MAX
    };
    int8_t cursor = C_RATE1;

public:
    OperatorEnvelopeScreen(uint8_t opIndex = 0) : operatorIndex(opIndex) {
        if (operatorIndex >= 6) operatorIndex = 0;
    }

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
            if (button == BTN_L_LONG) dir = -5;
            adjustParameter(dir);
            changed = true;
        }
        else if (button == BTN_R || button == BTN_R_LONG) {
            int8_t dir = 1;
            if (button == BTN_R_LONG) dir = 5;
            adjustParameter(dir);
            changed = true;
        }

        // ENTERボタン or CANCELボタン：戻る
        else if (button == BTN_ET || button == BTN_CXL) {
            if (cursor == C_BACK || button == BTN_CXL) {
                manager->popScreen();
                return;
            }
        }

        if (moved || changed) {
            if (changed) {
                needsFullRedraw = true;  // グラフも含めて再描画
            }
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
        char headerStr[20];
        sprintf(headerStr, "OP%d ENVELOPE", operatorIndex + 1);
        canvas.print(headerStr);
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
        manager->transferPartial(0, 0, SCREEN_WIDTH, HEADER_H + 1);
    }

    void drawAllItems(GFXcanvas16& canvas) {
        Synth& synth = Synth::getInstance();
        const Envelope& env = synth.getOperatorEnv(operatorIndex);

        char valueStr[8];

        // Rateパラメータ (左列)
        sprintf(valueStr, "%d", env.getRate1());
        drawParamItem(canvas, "R1", valueStr, 0, cursor == C_RATE1, false);

        sprintf(valueStr, "%d", env.getRate2());
        drawParamItem(canvas, "R2", valueStr, 1, cursor == C_RATE2, false);

        sprintf(valueStr, "%d", env.getRate3());
        drawParamItem(canvas, "R3", valueStr, 2, cursor == C_RATE3, false);

        sprintf(valueStr, "%d", env.getRate4());
        drawParamItem(canvas, "R4", valueStr, 3, cursor == C_RATE4, false);

        // Levelパラメータ (右列)
        sprintf(valueStr, "%d", env.getLevel1());
        drawParamItem(canvas, "L1", valueStr, 0, cursor == C_LEVEL1, true);

        sprintf(valueStr, "%d", env.getLevel2());
        drawParamItem(canvas, "L2", valueStr, 1, cursor == C_LEVEL2, true);

        sprintf(valueStr, "%d", env.getLevel3());
        drawParamItem(canvas, "L3", valueStr, 2, cursor == C_LEVEL3, true);

        sprintf(valueStr, "%d", env.getLevel4());
        drawParamItem(canvas, "L4", valueStr, 3, cursor == C_LEVEL4, true);

        // エンベロープグラフを描画
        drawEnvelopeGraph(canvas);
    }

    void drawFooter(GFXcanvas16& canvas) {
        canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
        drawBackButton(canvas, cursor == C_BACK);
    }

    void updateCursorElement(GFXcanvas16& canvas, int8_t cursorPos) {
        Synth& synth = Synth::getInstance();
        const Envelope& env = synth.getOperatorEnv(operatorIndex);
        bool isSelected = (cursor == cursorPos);
        char valueStr[8];

        switch (cursorPos) {
            case C_RATE1:
                sprintf(valueStr, "%d", env.getRate1());
                drawParamItem(canvas, "R1", valueStr, 0, isSelected, false);
                break;
            case C_RATE2:
                sprintf(valueStr, "%d", env.getRate2());
                drawParamItem(canvas, "R2", valueStr, 1, isSelected, false);
                break;
            case C_RATE3:
                sprintf(valueStr, "%d", env.getRate3());
                drawParamItem(canvas, "R3", valueStr, 2, isSelected, false);
                break;
            case C_RATE4:
                sprintf(valueStr, "%d", env.getRate4());
                drawParamItem(canvas, "R4", valueStr, 3, isSelected, false);
                break;
            case C_LEVEL1:
                sprintf(valueStr, "%d", env.getLevel1());
                drawParamItem(canvas, "L1", valueStr, 0, isSelected, true);
                break;
            case C_LEVEL2:
                sprintf(valueStr, "%d", env.getLevel2());
                drawParamItem(canvas, "L2", valueStr, 1, isSelected, true);
                break;
            case C_LEVEL3:
                sprintf(valueStr, "%d", env.getLevel3());
                drawParamItem(canvas, "L3", valueStr, 2, isSelected, true);
                break;
            case C_LEVEL4:
                sprintf(valueStr, "%d", env.getLevel4());
                drawParamItem(canvas, "L4", valueStr, 3, isSelected, true);
                break;
            case C_BACK:
                drawBackButton(canvas, isSelected);
                break;
        }
    }

    // 2列レイアウト用のパラメータ描画
    void drawParamItem(GFXcanvas16& canvas, const char* name, const char* value, int index, bool selected, bool rightColumn) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);
        int16_t x = rightColumn ? 64 : 0;
        int16_t w = 64;

        canvas.fillRect(x, y, w, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);

        if (selected) {
            canvas.fillRect(x + 2, y + 2, 3, 8, Color::WHITE);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(x + 8, y + 2);
        canvas.print(name);

        canvas.setCursor(x + 30, y + 2);
        canvas.setTextColor(Color::WHITE);
        canvas.print(value);

        manager->transferPartial(x, y, w, ITEM_H);
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

    /**
     * @brief エンベロープグラフを描画
     * DX7スタイルの4ポイントエンベロープを視覚化
     */
    void drawEnvelopeGraph(GFXcanvas16& canvas) {
        Synth& synth = Synth::getInstance();
        const Envelope& env = synth.getOperatorEnv(operatorIndex);

        const int16_t graphX = 2;
        const int16_t graphW = SCREEN_WIDTH - 4;

        // グラフエリアをクリア
        canvas.fillRect(graphX, GRAPH_Y, graphW, GRAPH_H, Color::BLACK);

        // レベル値を取得 (0-99)
        uint8_t L1 = env.getLevel1();
        uint8_t L2 = env.getLevel2();
        uint8_t L3 = env.getLevel3();
        uint8_t L4 = env.getLevel4();

        // レート値を取得 (0-99)
        uint8_t R1 = env.getRate1();
        uint8_t R2 = env.getRate2();
        uint8_t R3 = env.getRate3();
        uint8_t R4 = env.getRate4();

        // レートから横幅を計算（レート99=短い、0=長い）
        auto rateToWidth = [](uint8_t rate) -> int16_t {
            return 5 + ((99 - rate) * 25) / 99;
        };

        int16_t w1 = rateToWidth(R1);
        int16_t w2 = rateToWidth(R2);
        int16_t w3 = rateToWidth(R3);
        int16_t w4 = rateToWidth(R4);

        // 幅をグラフ幅に正規化
        int16_t totalW = w1 + w2 + w3 + w4;
        float scale = static_cast<float>(graphW - 4) / totalW;
        w1 = static_cast<int16_t>(w1 * scale);
        w2 = static_cast<int16_t>(w2 * scale);
        w3 = static_cast<int16_t>(w3 * scale);
        w4 = graphW - 4 - w1 - w2 - w3;

        // レベルからY座標を計算（レベル99=上、0=下）
        auto levelToY = [this](uint8_t level) -> int16_t {
            return GRAPH_Y + GRAPH_H - 2 - (level * (GRAPH_H - 4)) / 99;
        };

        int16_t y0 = GRAPH_Y + GRAPH_H - 2;
        int16_t y1 = levelToY(L1);
        int16_t y2 = levelToY(L2);
        int16_t y3 = levelToY(L3);
        int16_t y4 = levelToY(L4);

        int16_t x0 = graphX + 2;
        int16_t x1 = x0 + w1;
        int16_t x2 = x1 + w2;
        int16_t x3 = x2 + w3;
        int16_t x4 = x3 + w4;

        // エンベロープ線を描画
        uint16_t lineColor = Color::CYAN;
        canvas.drawLine(x0, y0, x1, y1, lineColor);  // Attack
        canvas.drawLine(x1, y1, x2, y2, lineColor);  // Decay1
        canvas.drawLine(x2, y2, x3, y3, lineColor);  // Decay2/Sustain
        canvas.drawLine(x3, y3, x4, y4, lineColor);  // Release

        // ポイントを描画
        uint16_t pointColor = Color::WHITE;
        canvas.fillCircle(x1, y1, 2, pointColor);
        canvas.fillCircle(x2, y2, 2, pointColor);
        canvas.fillCircle(x3, y3, 2, pointColor);
        canvas.fillCircle(x4, y4, 2, pointColor);

        manager->transferPartial(graphX, GRAPH_Y, graphW, GRAPH_H);
    }

    void adjustParameter(int8_t direction) {
        Synth& synth = Synth::getInstance();
        Envelope& env = const_cast<Envelope&>(synth.getOperatorEnv(operatorIndex));

        auto adjustValue = [](uint8_t current, int8_t dir) -> uint8_t {
            int16_t newVal = current + dir;
            if (newVal < 0) newVal = 0;
            if (newVal > 99) newVal = 99;
            return static_cast<uint8_t>(newVal);
        };

        switch (cursor) {
            case C_RATE1:
                env.setRate1(adjustValue(env.getRate1(), direction));
                break;
            case C_RATE2:
                env.setRate2(adjustValue(env.getRate2(), direction));
                break;
            case C_RATE3:
                env.setRate3(adjustValue(env.getRate3(), direction));
                break;
            case C_RATE4:
                env.setRate4(adjustValue(env.getRate4(), direction));
                break;
            case C_LEVEL1:
                env.setLevel1(adjustValue(env.getLevel1(), direction));
                break;
            case C_LEVEL2:
                env.setLevel2(adjustValue(env.getLevel2(), direction));
                break;
            case C_LEVEL3:
                env.setLevel3(adjustValue(env.getLevel3(), direction));
                break;
            case C_LEVEL4:
                env.setLevel4(adjustValue(env.getLevel4(), direction));
                break;
        }
    }
};

// ===== ピッチ設定画面 (MODE/TUNE/COARSE/FINE) =====
class OperatorPitchScreen : public Screen {
private:
    const int16_t HEADER_H = 12;
    const int16_t ITEM_H = 16;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 12;

    bool needsFullRedraw = false;
    uint8_t operatorIndex = 0;

    enum CursorPos {
        C_MODE = 0,   // RATIO/FIXED
        C_TUNE,       // Detune (±7)
        C_COARSE,     // 周波数比
        C_FINE,       // 微調整
        C_BACK,
        C_MAX
    };
    int8_t cursor = C_MODE;

public:
    OperatorPitchScreen(uint8_t opIndex = 0) : operatorIndex(opIndex) {
        if (operatorIndex >= 6) operatorIndex = 0;
    }

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
            int8_t dir = -1;
            if (button == BTN_L_LONG) dir = -5;
            adjustParameter(dir);
            changed = true;
        }
        else if (button == BTN_R || button == BTN_R_LONG) {
            int8_t dir = 1;
            if (button == BTN_R_LONG) dir = 5;
            adjustParameter(dir);
            changed = true;
        }
        else if (button == BTN_ET) {
            if (cursor == C_MODE) {
                // MODEをトグル
                Synth& synth = Synth::getInstance();
                Oscillator& osc = const_cast<Oscillator&>(synth.getOperatorOsc(operatorIndex));
                osc.setFixed(!osc.isFixed());
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
        char headerStr[20];
        sprintf(headerStr, "OP%d PITCH", operatorIndex + 1);
        canvas.print(headerStr);
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
        manager->transferPartial(0, 0, SCREEN_WIDTH, HEADER_H + 1);
    }

    void drawAllItems(GFXcanvas16& canvas) {
        Synth& synth = Synth::getInstance();
        const Oscillator& osc = synth.getOperatorOsc(operatorIndex);

        // MODE表示 (RATIO/FIXED)
        const char* modeStr = osc.isFixed() ? "FIXED" : "RATIO";
        drawTextItem(canvas, "MODE", modeStr, 0, cursor == C_MODE);

        // TUNE表示 (Detune ±7)
        char tuneStr[8];
        sprintf(tuneStr, "%+d", osc.getDetune());
        drawTextItem(canvas, "TUNE", tuneStr, 1, cursor == C_TUNE);

        // COARSE表示 (0-31)
        char coarseStr[8];
        sprintf(coarseStr, "%d", static_cast<int>(osc.getCoarse()));
        drawTextItem(canvas, "COARSE", coarseStr, 2, cursor == C_COARSE);

        // FINE表示
        char fineStr[8];
        sprintf(fineStr, "%.0f", osc.getFine());
        drawTextItem(canvas, "FINE", fineStr, 3, cursor == C_FINE);
    }

    void drawFooter(GFXcanvas16& canvas) {
        canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
        drawBackButton(canvas, cursor == C_BACK);
    }

    void updateCursorElement(GFXcanvas16& canvas, int8_t cursorPos) {
        Synth& synth = Synth::getInstance();
        const Oscillator& osc = synth.getOperatorOsc(operatorIndex);
        bool isSelected = (cursor == cursorPos);

        if (cursorPos == C_MODE) {
            const char* modeStr = osc.isFixed() ? "FIXED" : "RATIO";
            drawTextItem(canvas, "MODE", modeStr, 0, isSelected);
        }
        else if (cursorPos == C_TUNE) {
            char tuneStr[8];
            sprintf(tuneStr, "%+d", osc.getDetune());
            drawTextItem(canvas, "TUNE", tuneStr, 1, isSelected);
        }
        else if (cursorPos == C_COARSE) {
            char coarseStr[8];
            sprintf(coarseStr, "%d", static_cast<int>(osc.getCoarse()));
            drawTextItem(canvas, "COARSE", coarseStr, 2, isSelected);
        }
        else if (cursorPos == C_FINE) {
            char fineStr[8];
            sprintf(fineStr, "%.0f", osc.getFine());
            drawTextItem(canvas, "FINE", fineStr, 3, isSelected);
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
            canvas.fillRect(2, y + 4, 3, 8, Color::WHITE);
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
        Oscillator& osc = const_cast<Oscillator&>(synth.getOperatorOsc(operatorIndex));

        switch (cursor) {
            case C_MODE: {
                osc.setFixed(!osc.isFixed());
                break;
            }
            case C_TUNE: {
                // TUNE (Detune ±7)
                int8_t currentDetune = osc.getDetune();
                int8_t step = (direction == 1 || direction == -1) ? 1 : 3;
                int8_t newDetune = currentDetune + (direction > 0 ? step : -step);
                if (newDetune < -7) newDetune = -7;
                if (newDetune > 7) newDetune = 7;
                osc.setDetune(newDetune);
                break;
            }
            case C_COARSE: {
                // COARSE (0-31)
                int8_t currentCoarse = static_cast<int8_t>(osc.getCoarse());
                int8_t step = (direction == 1 || direction == -1) ? 1 : 5;
                int8_t newCoarse = currentCoarse + (direction > 0 ? step : -step);
                if (newCoarse < 0) newCoarse = 0;
                if (newCoarse > 31) newCoarse = 31;
                osc.setCoarse(static_cast<float>(newCoarse));
                break;
            }
            case C_FINE: {
                float currentFine = osc.getFine();
                float step = (direction == 1 || direction == -1) ? 1.0f : 5.0f;
                float newFine = currentFine + (direction > 0 ? step : -step);
                if (newFine < 0.0f) newFine = 0.0f;
                if (newFine > 99.0f) newFine = 99.0f;
                osc.setFine(newFine);
                break;
            }
        }
    }
};

class OperatorScreen : public Screen {
private:
    // 定数
    const int16_t HEADER_H = 12;
    const int16_t ITEM_H = 16;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 12;

    // 状態変数
    bool needsFullRedraw = false;
    uint8_t operatorIndex = 0; // 0-5 (OP1-6)

    // --- カーソル管理 ---
    enum CursorPos {
        C_ENABLED = 0,
        C_WAVE,
        C_LEVEL,
        C_PITCH,   // ピッチ設定サブメニュー
        C_ENV,     // エンベロープ設定
        C_BACK,
        C_MAX
    };
    int8_t cursor = C_ENABLED;

public:
    OperatorScreen(uint8_t opIndex = 0) : operatorIndex(opIndex) {
        if (operatorIndex >= 6) operatorIndex = 0;
    }

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
            if (button == BTN_L_LONG) dir = -5;
            adjustParameter(dir);
            changed = true;
        }
        else if (button == BTN_R || button == BTN_R_LONG) {
            int8_t dir = 1;
            if (button == BTN_R_LONG) dir = 5;
            adjustParameter(dir);
            changed = true;
        }

        // ENTERボタン：トグル or 遷移 or 戻る
        else if (button == BTN_ET) {
            if (cursor == C_ENABLED) {
                Synth& synth = Synth::getInstance();
                Oscillator& osc = const_cast<Oscillator&>(synth.getOperatorOsc(operatorIndex));
                if (osc.isEnabled()) {
                    osc.disable();
                } else {
                    osc.enable();
                }
                changed = true;
            }
            else if (cursor == C_PITCH) {
                // PITCH設定サブメニューへ遷移
                manager->pushScreen(new OperatorPitchScreen(operatorIndex));
                return;
            }
            else if (cursor == C_ENV) {
                manager->pushScreen(new OperatorEnvelopeScreen(operatorIndex));
                return;
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
        char headerStr[16];
        sprintf(headerStr, "OPERATOR %d", operatorIndex + 1);
        canvas.print(headerStr);
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
        manager->transferPartial(0, 0, SCREEN_WIDTH, HEADER_H + 1);
    }

    /**
     * @brief すべてのアイテムを描画
     */
    void drawAllItems(GFXcanvas16& canvas) {
        Synth& synth = Synth::getInstance();
        const Oscillator& osc = synth.getOperatorOsc(operatorIndex);

        drawToggleItem(canvas, "ENABLED", osc.isEnabled(), 0, cursor == C_ENABLED);

        const char* waveNames[] = {"SINE", "TRI", "SAW", "SQR"};
        uint8_t waveId = osc.getWavetableId();
        drawTextItem(canvas, "WAVE", waveNames[waveId], 1, cursor == C_WAVE);

        // LEVEL表示 (0-99)
        char levelStr[8];
        sprintf(levelStr, "%d", osc.getLevel());
        drawTextItem(canvas, "LEVEL", levelStr, 2, cursor == C_LEVEL);

        // PITCH表示 (サブメニュー)
        drawMenuItemWithArrow(canvas, "PITCH", 3, cursor == C_PITCH);

        // ENV表示 (サブメニュー)
        drawMenuItemWithArrow(canvas, "ENV", 4, cursor == C_ENV);
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
        const Oscillator& osc = synth.getOperatorOsc(operatorIndex);
        bool isSelected = (cursor == cursorPos);

        if (cursorPos == C_ENABLED) {
            drawToggleItem(canvas, "ENABLED", osc.isEnabled(), 0, isSelected);
        }
        else if (cursorPos == C_WAVE) {
            const char* waveNames[] = {"SINE", "TRI", "SAW", "SQR"};
            uint8_t waveId = osc.getWavetableId();
            drawTextItem(canvas, "WAVE", waveNames[waveId], 1, isSelected);
        }
        else if (cursorPos == C_LEVEL) {
            char levelStr[8];
            sprintf(levelStr, "%d", osc.getLevel());
            drawTextItem(canvas, "LEVEL", levelStr, 2, isSelected);
        }
        else if (cursorPos == C_PITCH) {
            drawMenuItemWithArrow(canvas, "PITCH", 3, isSelected);
        }
        else if (cursorPos == C_ENV) {
            drawMenuItemWithArrow(canvas, "ENV", 4, isSelected);
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
     * @brief テキストアイテムを描画
     */
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

    /**
     * @brief パラメータを調整
     * @param direction 増減方向と量
     */
    void adjustParameter(int8_t direction) {
        Synth& synth = Synth::getInstance();
        Oscillator& osc = const_cast<Oscillator&>(synth.getOperatorOsc(operatorIndex));

        switch (cursor) {
            case C_WAVE: {
                // 波形タイプを変更 (0-3)
                uint8_t currentWave = osc.getWavetableId();
                int8_t newWave = currentWave + direction;
                if (newWave < 0) newWave = 3;
                if (newWave > 3) newWave = 0;
                osc.setWavetable(newWave);
                break;
            }
            case C_LEVEL: {
                // レベルを変更 (0-99)
                int16_t currentLevel = static_cast<int16_t>(osc.getLevel());
                int16_t step = (direction == 1 || direction == -1) ? 1 : 5;
                int16_t newLevel = currentLevel + (direction > 0 ? step : -step);
                if (newLevel < 0) newLevel = 0;
                if (newLevel > 99) newLevel = 99;
                osc.setLevelNonLinear(static_cast<uint8_t>(newLevel));
                break;
            }
            case C_PITCH:
            case C_ENV:
                // サブメニューなのでここでは何もしない
                break;
        }
    }

    /**
     * @brief メニュー遷移アイテム（矢印付き）を描画
     */
    void drawMenuItemWithArrow(GFXcanvas16& canvas, const char* name, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);

        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);

        if (selected) {
            canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(name);

        canvas.setCursor(110, y + 4);
        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.print(">");

        manager->transferPartial(0, y, SCREEN_WIDTH, ITEM_H);
    }
};
