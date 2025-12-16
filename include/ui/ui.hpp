#pragma once

#include <stack>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <cmath>

#include "display/gfx.hpp"
#include "utils/state.hpp"
#include "ui/screens/screen.hpp"

// #include "tools/midi_player.hpp"

class UIManager {
private:
    std::stack<Screen*> screenStack; // 画面をポインタで管理するスタック
    GFXcanvas16 canvas;              // 描画用のキャンバス
    bool redraw = true;              // 再描画が必要かどうかのフラグ
    int8_t playing = 0; //TODO TEST
    State& state_;

public:
    UIManager(State& state)
        : canvas(SCREEN_WIDTH, SCREEN_HEIGHT), state_(state) {}

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
        //TODO デモ用----------------------------------------
        // if(button == BTN_ET){
        //     MIDIPlayer::stop();
        //     const char* a = "demo1.mid";
        //     const char* b = "demo2.mid";
        //     const char* c = "demo3.mid";
        //     if(playing == 0) MIDIPlayer::play(a);
        //     else if(playing == 1) MIDIPlayer::play(b);
        //     else if(playing == 2) MIDIPlayer::play(c);
        //     playing = (playing + 1) % 4;
        // }
        // if(button == BTN_CXL){
        //     MIDIPlayer::stop();
        //     playing = 0;
        // }
        //TODO ------------------------------------------------
    }

    // 描画処理（ループ）
    void render() {
        // Stateを確認してボタンが押されていたらhandleInputを呼び出す
        auto btn_state = state_.getBtnState();
        if (btn_state != BTN_NONE) {
            handleInput(btn_state);
            state_.setBtnState(BTN_NONE);
        }

        // 現在の画面を取得
        Screen* currentScreen = screenStack.empty() ? nullptr : screenStack.top();

        // 再描画判定
        bool shouldDraw = redraw || (currentScreen && currentScreen->isAnimated());
        if (!shouldDraw) return;

        // 描画実行
        if (!currentScreen) {
            canvas.fillScreen(Color::BLACK);
        }
        else {
            currentScreen->draw(canvas);
        }

        GFX_SSD1351::flash(canvas);

        redraw = false;
    }

    // 画面側から再描画を要求するためのメソッド
    void setRedraw() {
        redraw = true;
    }

    State& getState() {
        return state_;
    }
};