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
    State& state_;

    int8_t playing = 0; //TODO TEST

    bool renderRequired = true;
    bool fullTransferRequired = false;

    uint32_t lastFrameTime = 0;
    static constexpr uint32_t MIN_FRAME_TIME = 66; // 15FPS制限

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
        invalidate();
        triggerFullTransfer();
        lastFrameTime = 0;
    }

    // 表示されている画面を閉じて次の画面を表示
    void popScreen() {
        if (!screenStack.empty()) {
            delete screenStack.top();
            screenStack.pop();
            invalidate();
            triggerFullTransfer();
            lastFrameTime = 0;
        }
        if (!screenStack.empty()) {
            screenStack.top()->onEnter(this);
        }
    }

    // ボタン入力を受け取り現在の画面に伝える
    void handleInput(uint8_t button) {
        if (!screenStack.empty() && button != BTN_NONE) {
            screenStack.top()->handleInput(button);
            invalidate();
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

        // エンコーダー回転処理（FPS制限の影響を受けない）
        int16_t enc_delta = state_.consumeEncoderDelta();
        if (enc_delta != 0 && !screenStack.empty()) {
            screenStack.top()->handleEncoder(enc_delta);
            invalidate();
        }

        // FPS制限
        uint32_t now = millis();
        if (now - lastFrameTime < MIN_FRAME_TIME) {
            return;
        }
        lastFrameTime = now;

        // 現在の画面を取得
        Screen* currentScreen = screenStack.empty() ? nullptr : screenStack.top();

        // 再描画判定
        bool shouldDraw = renderRequired || (currentScreen && currentScreen->isAnimated());

        if (shouldDraw && currentScreen) {
            currentScreen->draw(canvas);
            renderRequired = false;
        }

        if (fullTransferRequired) {
            // 全画面転送
            if (!currentScreen) canvas.fillScreen(Color::BLACK);
            GFX_SSD1351::flash(canvas);
            fullTransferRequired = false;
        }
    }

    /**
     * @brief Canvasの再描画を要求 (draw()が呼ばれる)
     * ディスプレイへの転送は予約しない。Screen::draw内で明示する必要がある。
     */
    void invalidate() {
        renderRequired = true;
    }

    /**
     * @brief 次のフレームで「全画面」をディスプレイに転送予約する
     */
    void triggerFullTransfer() {
        fullTransferRequired = true;
    }

    void transferPartial(int16_t x, int16_t y, int16_t w, int16_t h) {
        if (!fullTransferRequired) {
            GFX_SSD1351::flashWindow(canvas, x, y, w, h);
        }
    }

    State& getState() {
        return state_;
    }
};