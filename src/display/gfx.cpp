#include "display/gfx.hpp"

/** @brief SSD1351初期化 */
void GFX_SSD1351::init() {
    display.begin();
    display.fillScreen(0x0000);
    // testdrawtext("RED", 0xF800);
    // testdrawtext("\nBLUE", 0x001F);
    // testdrawtext("\n\nGREEN", 0x07E0);
    // testdrawtext("\n\n\nCYAN", 0x07FF);
    // testdrawtext("\n\n\n\nMAGENTA", 0xF81F);
    // testdrawtext("\n\n\n\n\nYELLOW", 0xFFE0);
    // testdrawtext("\n\n\n\n\n\nWHITE", 0xFFFF);
    delay(500);
}

void GFX_SSD1351::testdrawtext(char *text, uint16_t color) {
    display.setCursor(0,0);
    display.setTextColor(color);
    display.print(text);
}