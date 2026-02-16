#pragma once

#include "ui/ui.hpp"
#include "ui/screens/passthrough.hpp"
#include "ui/screens/midi_player_screen.hpp"

class MenuScreen : public Screen {
private:
    const int16_t HEADER_H = 12;
    const int16_t ITEM_H = 16;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 12;

    bool needsFullRedraw = false;

    enum CursorPos {
        C_PASSTHROUGH = 0,
        C_MIDI_PLAYER,
        C_BACK,
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
            else if (cursor == C_MIDI_PLAYER) {
                manager->pushScreen(new MIDIPlayerScreen());
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
        canvas.print("MENU");
        canvas.drawFastHLine(0, HEADER_H, SCREEN_WIDTH, Color::WHITE);
    }

    void drawAllItems(GFXcanvas16& canvas) {
        drawNavItem(canvas, "PASSTHROUGH", 0, cursor == C_PASSTHROUGH);
        drawNavItem(canvas, "MIDI PLAYER", 1, cursor == C_MIDI_PLAYER);
    }

    void drawFooter(GFXcanvas16& canvas) {
        canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
        drawBackButton(canvas, cursor == C_BACK);
    }

    void updateCursorElement(GFXcanvas16& canvas, int8_t pos) {
        bool sel = (cursor == pos);
        switch (pos) {
            case C_PASSTHROUGH: drawNavItem(canvas, "PASSTHROUGH", 0, sel); break;
            case C_MIDI_PLAYER: drawNavItem(canvas, "MIDI PLAYER", 1, sel); break;
            case C_BACK:        drawBackButton(canvas, sel); break;
        }
    }

    void drawNavItem(GFXcanvas16& canvas, const char* name, int index, bool selected) {
        int16_t y = HEADER_H + 2 + (index * ITEM_H);

        canvas.fillRect(0, y, SCREEN_WIDTH, ITEM_H, Color::BLACK);
        canvas.setTextSize(1);

        if (selected) {
            canvas.fillRect(2, y + 2, 3, 8, Color::WHITE);
        }

        canvas.setTextColor(selected ? Color::WHITE : Color::MD_GRAY);
        canvas.setCursor(10, y + 4);
        canvas.print(name);

        // 右矢印でサブメニューを示す
        canvas.setCursor(110, y + 4);
        canvas.setTextColor(Color::MD_GRAY);
        canvas.print(">");

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
};
