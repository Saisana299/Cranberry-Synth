#pragma once

#include <stack>
#include <memory>
#include "screens/screen.hpp"
#include "display/gfx.hpp"
#include "utils/state.hpp"

class UIManager {
private:
    std::stack<Screen*> screenStack; // 画面をポインタで管理するスタック
    GFXcanvas16 canvas;              // 描画用のキャンバス
    bool redraw = true;              // 再描画が必要かどうかのフラグ

public:
    UIManager() : canvas(SCREEN_WIDTH, SCREEN_HEIGHT) {}

    // スタックに残っている画面を全て削除
    ~UIManager() {
        while (!screenStack.empty()) {
            delete screenStack.top();
            screenStack.pop();
        }
    }

    // 新しい画面を追加する
    void pushScreen(Screen* newScreen) {
        if (!screenStack.empty()) {
            screenStack.top()->onExit();
        }
        screenStack.push(newScreen);
        newScreen->onEnter(this);
        redraw = true;
    }

    // 表示されている画面を閉じて次の画面を表示
    void popScreen() {
        if (!screenStack.empty()) {
            delete screenStack.top();
            screenStack.pop();
            redraw = true;
        }
        if (!screenStack.empty()) {
            screenStack.top()->onEnter(this);
        }
    }

    // ボタン入力を受け取り現在の画面に伝える
    void handleInput(uint8_t button) {
        if (!screenStack.empty() && button != BTN_NONE) {
            screenStack.top()->handleInput(button);
            redraw = true;
        }
    }

    // 描画処理（ループ）
    void render() {
        // Stateを確認してボタンが押されていたらhandleInputを呼び出す
        if (State::btn_state != BTN_NONE) {
            handleInput(State::btn_state);
            State::btn_state = BTN_NONE;
        }

        // 再描画確認
        if (!redraw) return;
        if (screenStack.empty()) {
            canvas.fillScreen(Color::BLACK);
        }
        else {
            screenStack.top()->draw(canvas);
        }
        GFX_SSD1351::flash(canvas);
        redraw = false;
    }

    // 画面側から再描画を要求するためのメソッド
    void setRedraw() {
        redraw = true;
    }
};