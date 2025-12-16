#pragma once

#include "ui/ui.hpp"

class PresetScreen : public Screen {
private:
    uint32_t frameCount = 0;

public:

    PresetScreen() = default;

    bool isAnimated() const override { return false; }

    void handleInput(uint8_t button) override {
        if (button == BTN_CXL) {
            manager->popScreen();
        }
    }

    void draw(GFXcanvas16& canvas) override {
        canvas.fillScreen(Color::BLACK);
        GFX_SSD1351::drawString(canvas, "MAIN MENU", 10, 10, Color::WHITE);
        GFX_SSD1351::drawString(canvas, "> Preset", 10, 30, Color::WHITE);
        GFX_SSD1351::drawString(canvas, "  Settings", 10, 45, Color::MD_GRAY);
    }
};