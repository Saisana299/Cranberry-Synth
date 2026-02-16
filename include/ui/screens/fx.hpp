#pragma once

#include "ui/ui.hpp"
#include "ui/screens/delay.hpp"
#include "ui/screens/chorus.hpp"
#include "ui/screens/hpf.hpp"
#include "ui/screens/lpf.hpp"

class FXScreen : public Screen {
private:
    // 定数
    const int16_t HEADER_H = 14;
    const int16_t ITEM_H = 16;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 14;

    // 状態変数
    bool needsFullRedraw = false;

    // --- カーソル管理 ---
    enum CursorPos {
        C_DELAY = 0,
        C_CHORUS,
        C_HPF,
        C_LPF,
        C_BACK,
        C_MAX
    };
    int8_t cursor = C_DELAY;

public:
    FXScreen() = default;

    void onEnter(UIManager* manager) override {
        this->manager = manager;
        // カーソル位置は保持する（リセットしない）
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

        // ENTERボタン：選択して詳細画面へ
        else if (button == BTN_ET) {
            if (cursor == C_DELAY) {
                manager->pushScreen(new DelayScreen());
                return;
            }
            else if (cursor == C_CHORUS) {
                manager->pushScreen(new ChorusScreen());
                return;
            }
            else if (cursor == C_HPF) {
                manager->pushScreen(new HPFScreen());
                return;
            }
            else if (cursor == C_LPF) {
                manager->pushScreen(new LPFScreen());
                return;
            }
            else if (cursor == C_BACK) {
                // 前の画面に戻る
                manager->popScreen();
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
            drawAllFXItems(canvas);
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
        canvas.print("FX SETTINGS");
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
        manager->transferPartial(0, 0, SCREEN_WIDTH, HEADER_H + 1);
    }

    /**
     * @brief すべてのFXアイテムを描画
     */
    void drawAllFXItems(GFXcanvas16& canvas) {
        drawFXItem(canvas, "DELAY", 0, cursor == C_DELAY);
        drawFXItem(canvas, "CHORUS", 1, cursor == C_CHORUS);
        drawFXItem(canvas, "HPF", 2, cursor == C_HPF);
        drawFXItem(canvas, "LPF", 3, cursor == C_LPF);
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
        bool isSelected = (cursor == cursorPos);

        if (cursorPos == C_DELAY) {
            drawFXItem(canvas, "DELAY", 0, isSelected);
        }
        else if (cursorPos == C_CHORUS) {
            drawFXItem(canvas, "CHORUS", 1, isSelected);
        }
        else if (cursorPos == C_HPF) {
            drawFXItem(canvas, "HPF", 2, isSelected);
        }
        else if (cursorPos == C_LPF) {
            drawFXItem(canvas, "LPF", 3, isSelected);
        }
        else if (cursorPos == C_BACK) {
            drawBackButton(canvas, isSelected);
        }
    }

    /**
     * @brief FXアイテム（選択メニュー）を描画
     */
    void drawFXItem(GFXcanvas16& canvas, const char* name, int index, bool selected) {
        Synth& synth = Synth::getInstance();
        int16_t y = HEADER_H + 2 + (index * ITEM_H);

        // 有効状態を取得
        bool isEnabled = false;
        if (index == 0) isEnabled = synth.isDelayEnabled();
        else if (index == 1) isEnabled = synth.isChorusEnabled();
        else if (index == 2) isEnabled = synth.isHpfEnabled();
        else if (index == 3) isEnabled = synth.isLpfEnabled();

        // 背景クリア（行全体）
        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);

        // テキスト
        canvas.setTextSize(1);
        int16_t x = 18;

        // 選択時は小さいインジケータのみ
        if (selected) {
            canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        }

        // 有効状態インジケータ（小さい点）
        if (isEnabled) {
            canvas.fillCircle(10, y + 6, 2, Color::CYAN);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(x, y + 4);
        canvas.print(name);

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
