#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>

#include "utils/color.hpp"

constexpr uint16_t SCREEN_WIDTH = 128;
constexpr uint16_t SCREEN_HEIGHT = 128;

// MOSI1_PIN = 26, SCK1_PIN  = 27
constexpr int8_t DC_PIN = 40;
constexpr int8_t CS_PIN = 38;
constexpr int8_t RST_PIN = 41;

class GFX_SSD1351 {
private:
    static inline Adafruit_SSD1351 display = {SCREEN_WIDTH, SCREEN_HEIGHT, &SPI1, CS_PIN, DC_PIN, RST_PIN};
    void init();

public:
    GFX_SSD1351() {
        init();
    }

    static inline void drawString(String text, int16_t x, int16_t y, uint16_t color) {
        display.setCursor(x, y);
        display.setTextColor(color);
        display.print(text);
    }
};