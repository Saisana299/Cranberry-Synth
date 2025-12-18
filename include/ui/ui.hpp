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

// 転送モードの定義
enum TransferMode : uint8_t {
    TRANSFER_NONE = 0,
    TRANSFER_PARTIAL, // 部分転送
    TRANSFER_FULL     // 全画面転送
};

class UIManager {
private:
    std::stack<Screen*> screenStack; // 画面をポインタで管理するスタック
    GFXcanvas16 canvas;              // 描画用のキャンバス
    State& state_;

    int8_t playing = 0; //TODO TEST

    bool renderRequired = true;
    TransferMode transferMode = TRANSFER_FULL;

    struct Rect {
        int16_t x, y, w, h;
    } dirtyRect = {0, 0, 0, 0};

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
    }

    // 表示されている画面を閉じて次の画面を表示
    void popScreen() {
        if (!screenStack.empty()) {
            delete screenStack.top();
            screenStack.pop();
            invalidate();
            triggerFullTransfer();
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

        // 現在の画面を取得
        Screen* currentScreen = screenStack.empty() ? nullptr : screenStack.top();

        // 再描画判定
        bool shouldProcess = renderRequired || (currentScreen && currentScreen->isAnimated());

        if (shouldProcess && currentScreen) {
            currentScreen->draw(canvas);
            renderRequired = false;
        }

        if (transferMode == TRANSFER_FULL) {
            // 全画面転送
            if (!currentScreen) canvas.fillScreen(Color::BLACK);
            GFX_SSD1351::flash(canvas);
            resetTransferState();
        }
        else if (transferMode == TRANSFER_PARTIAL) {
            // 部分転送
            GFX_SSD1351::flashWindow(canvas, dirtyRect.x, dirtyRect.y, dirtyRect.w, dirtyRect.h);
            resetTransferState();
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
        transferMode = TRANSFER_FULL;
    }

    /**
     * @brief 次のフレームで「指定した範囲」をディスプレイに転送予約する
     * 1フレーム中に複数回呼ばれた場合は、それらを包含する最小矩形にマージされる
     */
    void markDirty(int16_t x, int16_t y, int16_t w, int16_t h) {
        // 全画面転送が既に予約されていたら、部分更新計算は不要
        if (transferMode == TRANSFER_FULL) return;

        if (transferMode == TRANSFER_NONE) {
            // まだ予約がない場合、そのまま設定
            dirtyRect = {x, y, w, h};
            transferMode = TRANSFER_PARTIAL;
        } else {
            // 既に部分予約がある場合、矩形を結合(Merge)する
            int16_t newX = std::min(dirtyRect.x, x);
            int16_t newY = std::min(dirtyRect.y, y);
            int16_t newMaxX = std::max((int16_t)(dirtyRect.x + dirtyRect.w), (int16_t)(x + w));
            int16_t newMaxY = std::max((int16_t)(dirtyRect.y + dirtyRect.h), (int16_t)(y + h));

            dirtyRect.x = newX;
            dirtyRect.y = newY;
            dirtyRect.w = newMaxX - newX;
            dirtyRect.h = newMaxY - newY;
        }
    }

    State& getState() {
        return state_;
    }

private:
    void resetTransferState() {
        transferMode = TRANSFER_NONE;
        dirtyRect = {0, 0, 0, 0};
    }
};