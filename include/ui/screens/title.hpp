#pragma once

#include <vector>
#include <string>
#include <functional>

#include "ui/screens/screen.hpp"
#include "ui/ui.hpp"
#include "utils/state.hpp"

class TitleScreen : public Screen {
public:

    TitleScreen() = default;

    void handleInput(uint8_t button) override {
        Serial.println(button);
        if (button == BTN_DN) {
        } else if (button == BTN_UP) {
        } else if (button == BTN_ET) {
        } else if (button == BTN_CXL) {
            // メニューを閉じる
            manager->popScreen();
        }
    }

    void draw(GFXcanvas16& canvas) override {
        TextBounds bounds = GFX_SSD1351::getTextBounds("Cranberry Synth", 0, 12);
        const uint16_t canvas_h = static_cast<uint16_t>(bounds.y) + bounds.h;
        GFXcanvas16 text_canvas{bounds.w, canvas_h};
        GFX_SSD1351::drawString(text_canvas, "Cranberry Synth", 0, 0, Color::GRAY);
        GFX_SSD1351::drawString(text_canvas, "Dev-1", 0, 12, Color::GRAY);
        GFX_SSD1351::drawCanvas(canvas, text_canvas, 0, 0);
    }
};