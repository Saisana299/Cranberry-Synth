#pragma once

#include "ui/ui.hpp"

class DelayScreen : public Screen {
private:
    const int16_t LINE_HEIGHT = 12;
    const int16_t CONTENT_Y = 16;
    int8_t cursor = 0;

    void redrawContent(GFXcanvas16& canvas) {
        Synth& synth = Synth::getInstance();
        canvas.fillRect(0, CONTENT_Y, SCREEN_WIDTH, SCREEN_HEIGHT - CONTENT_Y, Color::BLACK);

        int16_t y = CONTENT_Y + 2;
        const char* labels[] = {"Enable", "Time", "Level", "Feedback", "< Back"};

        for (int8_t i = 0; i < 5; i++) {
            // 値の表示文字列を作成
            char value[16] = "";
            if (i == 0) {
                strcpy(value, synth.isDelayEnabled() ? "ON" : "OFF");
            } else if (i == 1) {
                sprintf(value, "%ldms", (long)synth.getDelayTime());
            } else if (i == 2) {
                sprintf(value, "%d%%", (int)(synth.getDelayLevel() * 100 / 1024));
            } else if (i == 3) {
                sprintf(value, "%d%%", (int)(synth.getDelayFeedback() * 100 / 1024));
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

public:
    void onEnter(UIManager* mgr) override { manager = mgr; cursor = 0; manager->invalidate(); }
    bool isAnimated() const override { return false; }

    void handleInput(uint8_t btn) override {
        Synth& synth = Synth::getInstance();
        int32_t step = (btn == BTN_L_LONG || btn == BTN_R_LONG) ? 10 : 1;
        bool redraw = false;

        // カーソル移動
        if (btn == BTN_DN || btn == BTN_DN_LONG) {
            cursor = (cursor + 1) % 5;
            redraw = true;
        }
        else if (btn == BTN_UP || btn == BTN_UP_LONG) {
            cursor = (cursor - 1 + 5) % 5;
            redraw = true;
        }
        // 値を減らす
        else if (btn == BTN_L || btn == BTN_L_LONG) {
            if (cursor == 1) {
                synth.getDelay().setDelay(
                    constrain(synth.getDelayTime() - step, 1, 300),
                    synth.getDelayLevel(),
                    synth.getDelayFeedback()
                );
            }
            else if (cursor == 2) {
                synth.getDelay().setDelay(
                    synth.getDelayTime(),
                    constrain(synth.getDelayLevel() - step * 10, 0, 1024),
                    synth.getDelayFeedback()
                );
            }
            else if (cursor == 3) {
                synth.getDelay().setDelay(
                    synth.getDelayTime(),
                    synth.getDelayLevel(),
                    constrain(synth.getDelayFeedback() - step * 10, 0, 1024)
                );
            }
            redraw = (cursor >= 1 && cursor <= 3);
        }
        // 値を増やす
        else if (btn == BTN_R || btn == BTN_R_LONG) {
            if (cursor == 1) {
                synth.getDelay().setDelay(
                    constrain(synth.getDelayTime() + step, 1, 300),
                    synth.getDelayLevel(),
                    synth.getDelayFeedback()
                );
            }
            else if (cursor == 2) {
                synth.getDelay().setDelay(
                    synth.getDelayTime(),
                    constrain(synth.getDelayLevel() + step * 10, 0, 1024),
                    synth.getDelayFeedback()
                );
            }
            else if (cursor == 3) {
                synth.getDelay().setDelay(
                    synth.getDelayTime(),
                    synth.getDelayLevel(),
                    constrain(synth.getDelayFeedback() + step * 10, 0, 1024)
                );
            }
            redraw = (cursor >= 1 && cursor <= 3);
        }
        // 決定・戻る
        else if (btn == BTN_ET) {
            if (cursor == 4) {
                manager->popScreen();
                return;
            }
            else if (cursor == 0) {
                synth.setDelayEnabled(!synth.isDelayEnabled());
                redraw = true;
            }
        }

        if (redraw) {
            manager->invalidate();
        }
    }

    void draw(GFXcanvas16& canvas) override {
        static bool firstDraw = true;
        if (firstDraw) {
            canvas.fillScreen(Color::BLACK);
            canvas.setTextSize(1);
            canvas.setTextColor(Color::WHITE);
            canvas.setCursor(2, 4);
            canvas.print("Delay Settings");
            canvas.drawFastHLine(0, 14, SCREEN_WIDTH, Color::WHITE);
            manager->triggerFullTransfer();
            firstDraw = false;
        }
        redrawContent(canvas);
    }
};
