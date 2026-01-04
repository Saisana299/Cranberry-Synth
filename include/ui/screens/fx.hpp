#pragma once

#include "ui/ui.hpp"
#include "ui/screens/delay.hpp"
#include "ui/screens/lpf.hpp"
#include "ui/screens/hpf.hpp"

class FXScreen : public Screen {
private:
    const int16_t LINE_HEIGHT = 12;
    const int16_t CONTENT_Y = 16;
    int8_t cursor = 0;

public:
    void onEnter(UIManager* mgr) override {
        manager = mgr;
        cursor = 0;
        State& state = manager->getState();
        state.setModeState(MODE_SYNTH);
        manager->invalidate();
    }

    bool isAnimated() const override { return false; }

    void handleInput(uint8_t btn) override {
        // カーソル移動
        if (btn == BTN_DN || btn == BTN_DN_LONG) {
            cursor = (cursor + 1) % 4;
            manager->invalidate();
        }
        else if (btn == BTN_UP || btn == BTN_UP_LONG) {
            cursor = (cursor - 1 + 4) % 4;
            manager->invalidate();
        }
        // 決定・画面遷移
        else if (btn == BTN_ET) {
            if (cursor == 0) {
                manager->pushScreen(new DelayScreen());
            }
            else if (cursor == 1) {
                manager->pushScreen(new LPFScreen());
            }
            else if (cursor == 2) {
                manager->pushScreen(new HPFScreen());
            }
            else if (cursor == 3) {
                manager->popScreen();
            }
        }
    }

    void draw(GFXcanvas16& canvas) override {
        static bool firstDraw = true;

        // ヘッダーは初回のみ描画
        if (firstDraw) {
            canvas.fillScreen(Color::BLACK);
            canvas.setTextSize(1);
            canvas.setTextColor(Color::WHITE);
            canvas.setCursor(2, 4);
            canvas.print("FX Settings");
            canvas.drawFastHLine(0, 14, SCREEN_WIDTH, Color::WHITE);
            manager->triggerFullTransfer();
            firstDraw = false;
        }

        // メニュー項目の描画
        Synth& synth = Synth::getInstance();
        canvas.fillRect(0, CONTENT_Y, SCREEN_WIDTH, SCREEN_HEIGHT - CONTENT_Y, Color::BLACK);

        int16_t y = CONTENT_Y + 2;
        const char* labels[] = {"Delay", "Low-Pass Filter", "High-Pass Filter", "< Back"};

        for (int8_t i = 0; i < 4; i++) {
            // 各エフェクトの状態を取得
            char value[16] = "";
            if (i == 0) {
                strcpy(value, synth.isDelayEnabled() ? "ON" : "OFF");
            } else if (i == 1) {
                strcpy(value, synth.isLpfEnabled() ? "ON" : "OFF");
            } else if (i == 2) {
                strcpy(value, synth.isHpfEnabled() ? "ON" : "OFF");
            }

            // カーソル位置の描画
            if (i == cursor) {
                canvas.fillRect(0, y - 1, SCREEN_WIDTH, LINE_HEIGHT, Color::WHITE);
                canvas.setTextColor(Color::BLACK);
            } else {
                canvas.setTextColor(Color::WHITE);
            }

            // ラベルと値を描画
            canvas.setCursor(4, y + 2);
            canvas.print(labels[i]);

            if (strlen(value)) {
                canvas.setCursor(SCREEN_WIDTH - strlen(value) * 6 - 4, y + 2);
                canvas.print(value);
            }

            y += LINE_HEIGHT;
        }

        manager->transferPartial(0, CONTENT_Y, SCREEN_WIDTH, SCREEN_HEIGHT - CONTENT_Y);
    }
};
