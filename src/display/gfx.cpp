#include "display/gfx.hpp"

/** @brief SSD1351初期化 */
void GFX_SSD1351::init() {
    display.begin();
    display.fillScreen(Color::BLACK);
}