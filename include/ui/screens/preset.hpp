#pragma once

#include "ui/ui.hpp"

class PresetScreen : public Screen {
private:
    // 定数
    const int16_t OP_SIZE = 14;
    const int16_t GRID_W  = 20;
    const int16_t GRID_H  = 20;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 14;

    // 状態変数
    bool firstDraw = true;
    bool cursorMoved = false;

    uint8_t lastPolyCount = 0;

    // --- カーソル管理 ---
    enum CursorPos {
        C_PRESET = 0,
        C_ALGO,
        C_OP1, C_OP2, C_OP3, C_OP4, C_OP5, C_OP6,
        C_FX,
        C_POLY,
        C_MENU,
        C_MAX
    };
    int8_t currentCursor = C_PRESET;
    int8_t previousCursor = C_PRESET;

    // --- Synth状態のキャッシュ ---
    struct SynthState {
        uint8_t preset_id = 0;
        const char* preset_name = "";
        uint8_t algorithm_id = 0;
        uint8_t feedback = 0;

        // オペレーター状態 (有効/無効のみ)
        bool op_enabled[6] = {false};

        // エフェクト状態
        bool delay_enabled = false;
        bool lpf_enabled = false;
        bool hpf_enabled = false;
    } currentState, previousState;

    /**
     * @brief Synthクラスから現在の状態を読み込む
     */
    void updateStateFromSynth() {
        Synth& synth = Synth::getInstance();

        // 前回の状態を保存
        previousState = currentState;

        // 現在の状態を取得
        currentState.preset_id = synth.getCurrentPresetId();
        currentState.preset_name = synth.getCurrentPresetName();
        currentState.algorithm_id = synth.getCurrentAlgorithmId();
        currentState.feedback = synth.getFeedbackAmount();

        // オペレーター状態
        for (int i = 0; i < 6; i++) {
            currentState.op_enabled[i] = synth.getOperatorOsc(i).isEnabled();
        }

        // エフェクト状態
        currentState.delay_enabled = synth.isDelayEnabled();
        currentState.lpf_enabled = synth.isLpfEnabled();
        currentState.hpf_enabled = synth.isHpfEnabled();
    }

    /**
     * @brief 状態が変化した部分を検出して再描画
     */
    void updateChangedElements(GFXcanvas16& canvas) {
        // プリセット名/ID変更
        if (currentState.preset_id != previousState.preset_id) {
            drawPresetHeader(canvas);
        }

        // アルゴリズム変更
        if (currentState.algorithm_id != previousState.algorithm_id) {
            drawAlgorithmLabel(canvas, currentCursor == C_ALGO);
            // アルゴリズムが変わるとオペレーター配置も変わるので全体再描画
            drawAlgoDiagram(canvas);
        }

        // オペレーター有効/無効の変化
        for (int i = 0; i < 6; i++) {
            if (currentState.op_enabled[i] != previousState.op_enabled[i]) {
                drawOpBox(canvas, i, currentCursor == (C_OP1 + i));
            }
        }

        // エフェクト状態変化
        if (currentState.delay_enabled != previousState.delay_enabled ||
            currentState.lpf_enabled != previousState.lpf_enabled ||
            currentState.hpf_enabled != previousState.hpf_enabled) {
            drawFooterItem(canvas, "FX", 10, FOOTER_Y + 5, currentCursor == C_FX);
        }
    }

public:
    PresetScreen() = default;

    void onEnter(UIManager* manager) override {
        this->manager = manager;
        State& state = manager->getState();
        state.setModeState(MODE_SYNTH);
        firstDraw = true;
        cursorMoved = false;
        lastPolyCount = 0;
        currentCursor = C_PRESET;
        previousCursor = C_PRESET;

        // Synthから状態を読み込む
        updateStateFromSynth();
        previousState = currentState; // 初回は差分なし

        manager->invalidate();
    }

    bool isAnimated() const override { return true; }

    void handleInput(uint8_t button) override {
        bool moved = false;

        // 移動前の位置を保存
        int8_t oldCursor = currentCursor;

        if (button == BTN_DN || button == BTN_DN_LONG) {
            currentCursor++;
            if (currentCursor >= C_MAX) currentCursor = 0;
            moved = true;
        }
        else if (button == BTN_UP || button == BTN_UP_LONG) {
            currentCursor--;
            if (currentCursor < 0) currentCursor = C_MAX - 1;
            moved = true;
        }
        if (moved) {
            previousCursor = oldCursor; // 移動元を記憶
            cursorMoved = true;
            manager->invalidate();
        }
    }

    void draw(GFXcanvas16& canvas) override {
        // --- 0. Synthの状態を更新 ---
        updateStateFromSynth();

        // --- 1. 初回描画 (全画面) ---
        if (firstDraw) {
            canvas.fillScreen(Color::BLACK);

            // 各パーツを描画
            drawPresetHeader(canvas);

            // 装飾線と白帯
            canvas.drawFastHLine(0, 14, SCREEN_WIDTH, Color::WHITE);
            canvas.fillRect(0, 15, SCREEN_WIDTH, 10, Color::WHITE);

            drawAlgorithmLabel(canvas, (currentCursor == C_ALGO));
            canvas.drawFastHLine(0, 25, SCREEN_WIDTH, Color::WHITE);

            // オペレーター図形
            drawAlgoDiagram(canvas); // 内部で currentCursor を見てハイライト判定

            // フッター
            canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
            drawFooterItems(canvas); // FX, MENU, POLY

            firstDraw = false;
            cursorMoved = false;
            manager->triggerFullTransfer();
        }

        // --- 2. カーソル移動時の部分更新 ---
        if (cursorMoved) {
            // 移動元(previous) を「非選択」で再描画
            updateCursorElement(canvas, previousCursor);

            // 移動先(current) を「選択」で再描画
            updateCursorElement(canvas, currentCursor);

            cursorMoved = false;
        }

        // --- 3. Synth状態変化の検出と更新 ---
        updateChangedElements(canvas);

        // --- 4. 定期更新 (POLY数) ---
        uint8_t currentPoly = Synth::getInstance().getActiveNoteCount();

        if (currentPoly != lastPolyCount) {
            lastPolyCount = currentPoly;
            updatePolyDisplay(canvas, currentPoly);
        }
    }

private:

    /**
     * @brief 指定されたカーソル位置のUIパーツだけを再描画するディスパッチャ
     */
    void updateCursorElement(GFXcanvas16& canvas, int8_t cursorPos) {
        bool isSelected = (currentCursor == cursorPos);

        if (cursorPos == C_PRESET) {
            drawPresetHeader(canvas);
        }
        else if (cursorPos == C_ALGO) {
            drawAlgorithmLabel(canvas, isSelected);
        }
        else if (cursorPos >= C_OP1 && cursorPos <= C_OP6) {
            // オペレーター番号 (0-5)
            drawOpBox(canvas, cursorPos - C_OP1, isSelected);
        }
        else if (cursorPos == C_FX) {
            drawFooterItem(canvas, "FX", 10, FOOTER_Y + 5, isSelected);
        }
        else if (cursorPos == C_MENU) {
            String menuStr = "MENU";
            int16_t menuWidth = menuStr.length() * 6;
            drawFooterItem(canvas, menuStr, SCREEN_WIDTH - menuWidth - 4, FOOTER_Y + 5, isSelected);
        }
        else if (cursorPos == C_POLY) {
            updatePolyDisplay(canvas, lastPolyCount);
        }
    }

    // --- 個別の描画関数群 (内部で markDirty を呼ぶ) ---

    void drawPresetHeader(GFXcanvas16& canvas) {
        canvas.fillRect(0, 0, SCREEN_WIDTH, 14, Color::BLACK);

        canvas.setTextSize(1);
        canvas.setTextColor(Color::WHITE);
        canvas.setCursor(2, 4);

        // 実際のSynth状態から表示
        char headerStr[32];
        sprintf(headerStr, "%03d:%s",
                currentState.preset_id + 1,  // 1-indexed表示
                currentState.preset_name);
        canvas.print(headerStr);

        // 部分転送予約
        manager->transferPartial(0, 0, SCREEN_WIDTH, 14);
    }

    void drawAlgorithmLabel(GFXcanvas16& canvas, bool selected) {
        char algoStr[20];
        sprintf(algoStr, "Algorithm:%d", currentState.algorithm_id + 1); // 1-indexed

        int16_t textWidth = strlen(algoStr) * 6;
        int16_t x = (SCREEN_WIDTH - textWidth) / 2;
        int16_t y = 16;
        int16_t h = 8;

        if (selected) {
            canvas.fillRect(x - 2, y - 1, textWidth + 4, h + 2, Color::BLACK);
            canvas.setTextColor(Color::WHITE);
        } else {
            // 非選択時は白帯に戻す (白で塗りつぶす)
            canvas.fillRect(x - 2, y - 1, textWidth + 4, h + 2, Color::WHITE);
            canvas.setTextColor(Color::BLACK);
        }

        canvas.setCursor(x, y);
        canvas.print(algoStr);

        manager->transferPartial(x - 2, y - 1, textWidth + 4, h + 2);
    }

    void drawOpBox(GFXcanvas16& canvas, int opIndex, bool selected) {
        // 座標計算（drawAlgoDiagramと同じロジック）
        // 実際のアルゴリズムIDを使用
        const Algorithm& algo = Algorithms::get(currentState.algorithm_id);

        // 全体サイズ計算
        int16_t minCol = 32000, maxCol = -32000;
        int16_t minRow = 32000, maxRow = -32000;
        for (int i = 0; i < MAX_OPERATORS; ++i) {
            if (algo.positions[i].col < minCol) minCol = algo.positions[i].col;
            if (algo.positions[i].col > maxCol) maxCol = algo.positions[i].col;
            if (algo.positions[i].row < minRow) minRow = algo.positions[i].row;
            if (algo.positions[i].row > maxRow) maxRow = algo.positions[i].row;
        }
        int16_t totalWidth = (maxCol - minCol) * GRID_W + OP_SIZE;
        int16_t totalHeight = (maxRow - minRow) * GRID_H + OP_SIZE;
        int16_t displayAreaTop = 26;
        int16_t displayAreaHeight = SCREEN_HEIGHT - displayAreaTop;
        int16_t originX = (SCREEN_WIDTH - totalWidth) / 2 - (minCol * GRID_W);
        int16_t originY = displayAreaTop + (displayAreaHeight - totalHeight) / 2 - (minRow * GRID_H) - 10;

        // 個別位置
        int16_t x = originX + algo.positions[opIndex].col * GRID_W;
        int16_t y = originY + algo.positions[opIndex].row * GRID_H;

        // オペレーターの有効/無効を反映
        bool isEnabled = currentState.op_enabled[opIndex];

        // 描画
        if (selected) {
            canvas.fillRect(x, y, OP_SIZE, OP_SIZE, Color::WHITE);
            canvas.drawRect(x, y, OP_SIZE, OP_SIZE, Color::WHITE);
            canvas.setTextColor(Color::BLACK);
        } else {
            // 無効なオペレーターはグレーアウト
            if (isEnabled) {
                canvas.fillRect(x, y, OP_SIZE, OP_SIZE, Color::BLACK);
                canvas.drawRect(x, y, OP_SIZE, OP_SIZE, Color::WHITE);
                canvas.setTextColor(Color::WHITE);
            } else {
                canvas.fillRect(x, y, OP_SIZE, OP_SIZE, Color::BLACK);
                canvas.drawRect(x, y, OP_SIZE, OP_SIZE, Color::MD_GRAY);
                canvas.setTextColor(Color::MD_GRAY);
            }
        }
        canvas.setCursor(x + 4, y + 3);
        canvas.print(opIndex + 1);

        manager->transferPartial(x, y, OP_SIZE, OP_SIZE);
    }

    void drawFooterItem(GFXcanvas16& canvas, const String& str, int16_t x, int16_t y, bool selected) {
        int16_t w = str.length() * 6;
        int16_t h = 8;
        // 背景クリア範囲
        int16_t bgX = x - 1;
        int16_t bgY = y - 1;
        int16_t bgW = w + 2;
        int16_t bgH = h + 2;

        // FXの場合、有効なエフェクトがあるかチェック
        bool hasFx = (str == "FX") &&
                     (currentState.delay_enabled ||
                      currentState.lpf_enabled ||
                      currentState.hpf_enabled);

        if (selected) {
            canvas.fillRect(bgX, bgY, bgW, bgH, Color::WHITE);
            canvas.setTextColor(Color::BLACK);
        } else {
            canvas.fillRect(bgX, bgY, bgW, bgH, Color::BLACK);
            // エフェクトが有効な場合は明るく表示
            canvas.setTextColor(hasFx ? Color::CYAN : Color::WHITE);
        }
        canvas.setCursor(x, y);
        canvas.print(str);

        manager->transferPartial(bgX, bgY, bgW, bgH);
    }

    void drawFooterItems(GFXcanvas16& canvas) {
        // FX
        drawFooterItem(canvas, "FX", 10, FOOTER_Y + 5, (currentCursor == C_FX));

        // MENU
        String menuStr = "MENU";
        int16_t menuWidth = menuStr.length() * 6;
        drawFooterItem(canvas, menuStr, SCREEN_WIDTH - menuWidth - 4, FOOTER_Y + 5, (currentCursor == C_MENU));

        // 区切り線
        canvas.drawFastVLine(30, FOOTER_Y + 1, 14, Color::MD_GRAY);
        canvas.drawFastVLine(SCREEN_WIDTH - 34, FOOTER_Y + 1, 14, Color::MD_GRAY);

        // POLY
        updatePolyDisplay(canvas, lastPolyCount);
    }

    /**
     * @brief POLY部分の描画・更新
     */
    void updatePolyDisplay(GFXcanvas16& canvas, uint8_t count) {
        // POLY表示内容の作成
        enum VoiceMode { V_POLY, V_MONO, V_UNISON };
        VoiceMode vMode = V_POLY; // 仮
        String centerStr;
        switch(vMode) {
            case V_POLY:   centerStr = "POLY:" + String(count); break;
            case V_MONO:   centerStr = "MONO"; break;
            case V_UNISON: centerStr = "UNISON"; break;
        }

        int16_t strWidth = centerStr.length() * 6;
        int16_t centerX = SCREEN_WIDTH / 2;
        int16_t textY = FOOTER_Y + 5;
        int16_t x = centerX - (strWidth / 2);

        // drawItemを使って描画（カーソル状態を反映）
        bool isSelected = (currentCursor == C_POLY);

        // 背景クリアサイズ
        int16_t clearW = 50;
        int16_t clearH = 10;
        int16_t clearX = centerX - 25;

        // カーソル選択中なら白背景、そうでなければ黒背景で塗りつぶし
        if (isSelected) {
            canvas.fillRect(clearX, textY - 1, clearW, clearH, Color::WHITE);
            canvas.setTextColor(Color::BLACK);
        } else {
            canvas.fillRect(clearX, textY - 1, clearW, clearH, Color::BLACK);
            canvas.setTextColor(Color::WHITE);
        }

        canvas.setCursor(x, textY);
        canvas.print(centerStr);

        // 部分転送要求
        manager->transferPartial(clearX, textY - 1, clearW, clearH);
    }

    /**
     * @brief アルゴリズム図を描画するヘルパー関数
     * @param canvas 描画対象のキャンバス
     */
    void drawAlgoDiagram(GFXcanvas16& canvas) {
        // 実際のアルゴリズムIDを使用
        const Algorithm& algo = Algorithms::get(currentState.algorithm_id);

        // 色設定
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

        // ボックスを描画 (drawOpBoxを利用)
        for (int i = 0; i < MAX_OPERATORS; i++) {
            bool isSel = (currentCursor == (C_OP1 + i));
            drawOpBox(canvas, i, isSel);
        }
    }
};