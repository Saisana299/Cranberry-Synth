#pragma once

#include "ui/ui.hpp"

class PresetScreen : public Screen {
private:
    // オペレーター図形
    const int16_t OP_SIZE = 14;
    const int16_t GRID_W  = 20;
    const int16_t GRID_H  = 20;

public:
    PresetScreen() = default;

    void onEnter(UIManager* manager) override {
        this->manager = manager;
        State& state = manager->getState();
        state.setModeState(MODE_SYNTH);
    }

    bool isAnimated() const override { return false; }

    void handleInput(uint8_t button) override {
        if (button == BTN_L || button == BTN_L_LONG || button == BTN_EC_L) {
            //
        }
        else if (button == BTN_R || button == BTN_R_LONG || button == BTN_EC_R) {
            //
        }
    }

    void draw(GFXcanvas16& canvas) override {
        canvas.fillScreen(Color::BLACK);

        canvas.setTextSize(1);
        canvas.setTextColor(Color::WHITE);
        canvas.setCursor(2, 4);

        char headerStr[32];
        sprintf(headerStr, "001:Sine Wave");
        canvas.print(headerStr);

        // ヘッダー下の線
        canvas.drawFastHLine(0, 14, SCREEN_WIDTH, Color::WHITE);

        canvas.fillRect(0, 15, SCREEN_WIDTH, 10, Color::WHITE);

        // 2. 文字列の準備
        int algoNum = 5;
        char algoStr[20];
        sprintf(algoStr, "Algorithm:%d", algoNum);

        // 3. 中央位置の計算 (標準フォントは横幅6pxと仮定)
        // 文字数 * 6px = 文字列全体の幅
        int16_t textWidth = strlen(algoStr) * 6;
        int16_t x = (SCREEN_WIDTH - textWidth) / 2;
        int16_t y = 16;

        // 4. 黒文字で描画
        canvas.setTextColor(Color::BLACK);
        canvas.setCursor(x, y);
        canvas.print(algoStr);

        // アルゴリズム番号表示

        canvas.drawFastHLine(0, 25, SCREEN_WIDTH, Color::WHITE);

        drawAlgoDiagram(canvas);

        // 区切り線 (Y = 114付近)
        int16_t footerLineY = SCREEN_HEIGHT - 14;
        canvas.drawFastHLine(0, footerLineY, SCREEN_WIDTH, Color::WHITE);

        // テキスト設定
        canvas.setTextSize(1);
        canvas.setTextColor(Color::WHITE);

        // テキストのY座標 (高さ14pxの中央に配置: 14/2 - 4(文字高さの半分) = +3px)
        int16_t textY = footerLineY + 5;

        // 1. 左側: FX
        canvas.setCursor(10, textY); // 左端から少し隙間を空ける
        canvas.print("FX");

        // 2. 中央: ボイスモード / 発音数
        // ※ 本来は state.getVoiceMode() などから取得
        // ここでは仮の変数を使用しています
        enum VoiceMode { V_POLY, V_MONO, V_UNISON };
        VoiceMode vMode = V_POLY;
        uint8_t polyCount = 0;

        String centerStr;
        switch(vMode) {
            case V_POLY:
                centerStr = "POLY:" + String(polyCount);
                break;
            case V_MONO:
                centerStr = "MONO";
                break;
            case V_UNISON:
                centerStr = "UNISON";
                break;
        }

        // 中央揃えの計算 (標準フォントは1文字6px幅と仮定)
        int16_t strWidth = centerStr.length() * 6;
        int16_t centerX = (SCREEN_WIDTH - strWidth) / 2;

        canvas.setCursor(centerX, textY);
        canvas.print(centerStr);

        // 3. 右側: MENU
        String menuStr = "MENU";
        int16_t menuWidth = menuStr.length() * 6;

        // 右端から少し隙間を空ける
        canvas.setCursor(SCREEN_WIDTH - menuWidth - 4, textY);
        canvas.print(menuStr);

        // FXとCenterの間
        canvas.drawFastVLine(30, footerLineY + 1, 14, Color::MD_GRAY);
        // CenterとMENUの間
        canvas.drawFastVLine(SCREEN_WIDTH - 34, footerLineY + 1, 14, Color::MD_GRAY);
    }
private:
    /**
     * @brief アルゴリズム図を描画するヘルパー関数
     * @param canvas 描画対象のキャンバス
     */
    void drawAlgoDiagram(GFXcanvas16& canvas) {
        const Algorithm& algo = Algorithms::get(0);

        // 色設定
        uint16_t opColor = Color::WHITE;
        uint16_t lineColor = Color::MD_GRAY;

        // 1. 図全体のサイズ計算
        int16_t minCol = 32000, maxCol = -32000;
        int16_t minRow = 32000, maxRow = -32000;

        for (int i = 0; i < MAX_OPERATORS; ++i) {
            if (algo.positions[i].col < minCol) minCol = algo.positions[i].col;
            if (algo.positions[i].col > maxCol) maxCol = algo.positions[i].col;
            if (algo.positions[i].row < minRow) minRow = algo.positions[i].row;
            if (algo.positions[i].row > maxRow) maxRow = algo.positions[i].row;
        }

        // オペレーター部分のサイズ
        int16_t totalWidth = (maxCol - minCol) * GRID_W + OP_SIZE;
        int16_t totalHeight = (maxRow - minRow) * GRID_H + OP_SIZE;

        // 2. センタリング計算
        int16_t displayAreaTop = 26; // 線の1px下
        int16_t displayAreaHeight = SCREEN_HEIGHT - displayAreaTop;

        // バスが下に伸びる分(GRID_H)を考慮して、全体を少し上にずらす調整 (-10程度)
        int16_t originX = (SCREEN_WIDTH - totalWidth) / 2 - (minCol * GRID_W);
        int16_t originY = displayAreaTop + (displayAreaHeight - totalHeight) / 2 - (minRow * GRID_H) - 10;

        // 3. 描画
        // --- A. オペレーター間の接続線 ---
        for (int dst = 0; dst < MAX_OPERATORS; dst++) {
            // src -> dst への接続があるかチェック
            for (int src = 0; src < MAX_OPERATORS; src++) {
                // mod_maskのビットが立っていたら接続されている
                if (algo.mod_mask[dst] & (1 << src)) {
                    int16_t x1 = originX + algo.positions[src].col * GRID_W + OP_SIZE / 2;
                    int16_t y1 = originY + algo.positions[src].row * GRID_H + OP_SIZE / 2;
                    int16_t x2 = originX + algo.positions[dst].col * GRID_W + OP_SIZE / 2;
                    int16_t y2 = originY + algo.positions[dst].row * GRID_H + OP_SIZE / 2;
                    canvas.drawLine(x1, y1, x2, y2, lineColor);
                }
            }
        }

        // --- B. フィードバックループの描画 (修正版) ---
        if (algo.feedback_op >= 0 && algo.feedback_op < MAX_OPERATORS) {
            int opIdx = algo.feedback_op;
            int16_t x = originX + algo.positions[opIdx].col * GRID_W;
            int16_t y = originY + algo.positions[opIdx].row * GRID_H;

            // コの字型のループ線 (右から出て上に戻る)
            int16_t loopOffset = 5;
            int16_t startX = x + OP_SIZE;
            int16_t startY = y + OP_SIZE / 2;
            int16_t endX = x + OP_SIZE / 2;
            int16_t endY = y;
            int16_t rightX = x + OP_SIZE + loopOffset;
            int16_t topY = y - loopOffset;

            canvas.drawLine(startX, startY, rightX, startY, lineColor); // 右へ
            canvas.drawLine(rightX, startY, rightX, topY, lineColor);   // 上へ
            canvas.drawLine(rightX, topY, endX, topY, lineColor);       // 左へ
            canvas.drawLine(endX, topY, endX, endY, lineColor);         // 下へ(戻る)
        }

        // --- C. キャリアからマスターバスへの接続線 ---
        int16_t busY = 0;
        int16_t minBusX = 32000;
        int16_t maxBusX = -32000;
        int carrierCount = 0;

        for (int i = 0; i < MAX_OPERATORS; i++) {
            // output_mask のビットが立っていたらキャリア
            if (algo.output_mask & (1 << i)) {
                carrierCount++;
                int16_t cx = originX + algo.positions[i].col * GRID_W + OP_SIZE / 2;
                int16_t cy = originY + algo.positions[i].row * GRID_H + OP_SIZE;

                // オペレーター間の隙間と同じ長さに設定
                int16_t dropLen = GRID_H - OP_SIZE;

                canvas.drawLine(cx, cy, cx, cy + dropLen, lineColor);

                busY = cy + dropLen;
                if (cx < minBusX) minBusX = cx;
                if (cx > maxBusX) maxBusX = cx;
            }
        }

        // 複数のキャリアがある場合のみ水平線(マスターバス)を描画
        if (carrierCount > 1) {
            canvas.drawLine(minBusX, busY, maxBusX, busY, lineColor);
        }

        // --- D. オペレーター本体 ---
        canvas.setTextSize(1);
        canvas.setTextColor(opColor);

        for (int i = 0; i < MAX_OPERATORS; i++) {
            int16_t x = originX + algo.positions[i].col * GRID_W;
            int16_t y = originY + algo.positions[i].row * GRID_H;

            // 線を隠すために中を黒塗り
            canvas.fillRect(x, y, OP_SIZE, OP_SIZE, Color::BLACK);

            // 枠線
            canvas.drawRect(x, y, OP_SIZE, OP_SIZE, opColor);

            // 番号 (0始まりのindexを+1して表示)
            canvas.setCursor(x + 4, y + 3);
            canvas.print(i + 1);
        }
    }
};