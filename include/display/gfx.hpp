#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>

#include "utils/color.hpp"

// WaveShare 1.5inch RGB OLED Module 128x128 16bit
constexpr uint16_t SCREEN_WIDTH = 128;
constexpr uint16_t SCREEN_HEIGHT = 128;

// MOSI, SCKはSPI1に接続 (26, 27)
constexpr int8_t CS_PIN = 38;
constexpr int8_t DC_PIN = 40;
constexpr int8_t RST_PIN = 41;

constexpr uint32_t OLED_SPI_SPEED = 30000000;

// フォントの高さ
constexpr int16_t DEFAULT_FONT_HEIGHT = 8;

struct TextBounds{
    int16_t x, y;
    uint16_t w, h;
};

class GFX_SSD1351 {
private:
    static inline Adafruit_SSD1351 display = {SCREEN_WIDTH, SCREEN_HEIGHT, &SPI1, CS_PIN, DC_PIN, RST_PIN};

public:
    static inline void begin() {
        display.begin(OLED_SPI_SPEED);
        display.setRotation(0);
        display.fillScreen(Color::BLACK);
    }

    /**
     * @brief キャンバスの内容をディスプレイに転送
     */
    static inline void flash(GFXcanvas16& canvas, int16_t x = 0, int16_t y = 0) {
        const int16_t w = canvas.width();
        const int16_t h = canvas.height();
        display.startWrite();
        display.drawRGBBitmap(x, y, canvas.getBuffer(), w, h);
        display.endWrite();
    }

    /**
     * @brief キャンバスの指定した矩形範囲だけをディスプレイに転送（部分更新）
     * @param canvas 転送元のキャンバス
     * @param x 転送開始X座標
     * @param y 転送開始Y座標
     * @param w 転送幅
     * @param h 転送高さ
     */
    static inline void flashWindow(GFXcanvas16& canvas, int16_t x, int16_t y, int16_t w, int16_t h) {
        // 範囲チェック
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x + w > canvas.width()) w = canvas.width() - x;
        if (y + h > canvas.height()) h = canvas.height() - y;
        if (w <= 0 || h <= 0) return;

        display.startWrite();

        // メモリ配置が不連続なため、1行ずつ転送する
        // (drawRGBBitmapはポインタ位置から連続したデータをw*h個描画するため)
        for(int16_t row = 0; row < h; ++row) {
            // キャンバス内の該当行の先頭ポインタを計算
            // buffer + (Y座標 * 全体幅) + X座標
            uint16_t* ptr = canvas.getBuffer() + ((y + row) * canvas.width()) + x;

            // 1行分だけ描画 (高さ1)
            display.drawRGBBitmap(x, y + row, ptr, w, 1);
        }

        display.endWrite();
    }

    /**
     * @brief 文字列を描画
     */
    static inline void drawString(GFXcanvas16& canvas, const String& text, int16_t x = 0, int16_t y = 0, uint16_t color = Color::WHITE, bool fill = true) {
        if(fill) {
            TextBounds bounds = getTextBounds(canvas, text, x, y);
            uint16_t clear_width = bounds.w > 0 ? bounds.w + 2 : canvas.width();
            canvas.fillRect(x, y, clear_width, DEFAULT_FONT_HEIGHT, Color::BLACK);
        }
        canvas.setCursor(x, y);
        canvas.setTextColor(color);
        canvas.print(text);
    }

    /**
     * @brief 文字列の描画範囲を取得
     */
    static inline TextBounds getTextBounds(const String& text, int16_t x = 0, int16_t y = 0) {
        TextBounds bounds;
        display.getTextBounds(text, x, y, &bounds.x, &bounds.y, &bounds.w, &bounds.h);
        return bounds;
    }
    static inline TextBounds getTextBounds(GFXcanvas16& canvas, const String& text, int16_t x = 0, int16_t y = 0) {
        TextBounds bounds;
        canvas.getTextBounds(text, x, y, &bounds.x, &bounds.y, &bounds.w, &bounds.h);
        return bounds;
    }

    /**
     * @brief 別のキャンバスにキャンバスを描画
     */
    static inline void drawCanvas(GFXcanvas16& target, GFXcanvas16& canvas, int16_t x = 0, int16_t y = 0) {
        target.drawRGBBitmap(x, y, canvas.getBuffer(), canvas.width(), canvas.height());
    }
};