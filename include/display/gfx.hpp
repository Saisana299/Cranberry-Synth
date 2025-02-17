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

struct TextBounds{
    int16_t x, y;
    uint16_t w, h;
};

class GFX_SSD1351 {
private:
    static inline Adafruit_SSD1351 display = {SCREEN_WIDTH, SCREEN_HEIGHT, &SPI1, CS_PIN, DC_PIN, RST_PIN};
    static inline GFXcanvas16 max_canvas = {SCREEN_WIDTH, SCREEN_HEIGHT};

public:
    static inline void begin() {
        display.begin();
        display.setRotation(3);
        display.fillScreen(Color::BLACK);
    }

    static inline void flash(GFXcanvas16& canvas, int16_t x = 0, int16_t y = 0) {
        const int16_t w = canvas.width();
        const int16_t h = canvas.height();
        display.startWrite();
        display.drawRGBBitmap(x, y, canvas.getBuffer(), w, h);
        display.endWrite();
    }

    static inline void drawString(GFXcanvas16& canvas, String text, int16_t x = 0, int16_t y = 0, uint16_t color = Color::WHITE) {
        canvas.fillRect(x, y, canvas.width(), 8, Color::BLACK);
        canvas.setCursor(x, y);
        canvas.setTextColor(color);
        canvas.print(text);
    }

    static inline TextBounds getTextBounds(String text, int16_t x = 0, int16_t y = 0) {
        TextBounds bounds;
        max_canvas.getTextBounds(text, x, y, &bounds.x, &bounds.y, &bounds.w, &bounds.h);
        return bounds;
    }

    static inline TextBounds getTextBounds(GFXcanvas16& canvas, String text, int16_t x = 0, int16_t y = 0) {
        TextBounds bounds;
        canvas.getTextBounds(text, x, y, &bounds.x, &bounds.y, &bounds.w, &bounds.h);
        return bounds;
    }
};