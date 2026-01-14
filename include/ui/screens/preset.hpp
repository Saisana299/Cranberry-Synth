#pragma once

#include "ui/ui.hpp"
#include "ui/screens/fx.hpp"
#include "ui/screens/operator.hpp"
#include "ui/screens/master.hpp"

class PresetScreen : public Screen {
private:
    // 定数
    const int16_t OP_SIZE = 14;
    const int16_t GRID_W  = 20;
    const int16_t GRID_H  = 20;
    const int16_t FOOTER_Y = SCREEN_HEIGHT - 14;

    // 状態変数
    uint8_t lastPolyCount = 0;
    bool needsFullRedraw = false;

    // カーソル位置
    enum CursorPos {
        C_PRESET = 0,
        C_ALGO,
        C_OP1, C_OP2, C_OP3, C_OP4, C_OP5, C_OP6,
        C_MASTER,
        C_FX,
        C_POLY,
        C_MENU,
        C_MAX
    };
    int8_t cursor = C_PRESET;

public:
    PresetScreen() = default;

    void onEnter(UIManager* manager) override {
        this->manager = manager;
        State& state = manager->getState();
        state.setModeState(MODE_SYNTH);
        lastPolyCount = 0;
        needsFullRedraw = true;

        // draw()内のstatic変数をリセットするため、次の描画で全体再描画させる
        manager->invalidate();
        manager->triggerFullTransfer();
    }

    bool isAnimated() const override { return true; }

    void handleInput(uint8_t button) override {
        bool moved = false;
        Synth& synth = Synth::getInstance();

        // カーソル移動（上下）
        if (button == BTN_DN || button == BTN_DN_LONG) {
            cursor++;
            if (cursor >= C_MAX) cursor = 0;
            moved = true;
        }
        else if (button == BTN_UP || button == BTN_UP_LONG) {
            cursor--;
            if (cursor < 0) cursor = C_MAX - 1;
            moved = true;
        }

        // 左右ボタン：値の変更
        else if (button == BTN_L || button == BTN_L_LONG) {
            if (cursor == C_PRESET) {
                // プリセット変更（前へ）
                uint8_t preset_id = synth.getCurrentPresetId();
                if (preset_id > 0) {
                    preset_id--;
                } else {
                    preset_id = 31; // 最後のプリセットへ
                }
                synth.reset(); // ノートをリセット
                synth.loadPreset(preset_id);
                needsFullRedraw = true;
                manager->invalidate();
            }
            else if (cursor == C_ALGO) {
                // アルゴリズム変更（前へ）
                uint8_t algo_id = synth.getCurrentAlgorithmId();
                if (algo_id > 0) {
                    algo_id--;
                } else {
                    algo_id = 31; // 最後のアルゴリズムへ
                }
                synth.reset(); // ノートをリセット
                synth.setAlgorithm(algo_id);
                needsFullRedraw = true;
                manager->invalidate();
            }
        }
        else if (button == BTN_R || button == BTN_R_LONG) {
            if (cursor == C_PRESET) {
                // プリセット変更（次へ）
                uint8_t preset_id = synth.getCurrentPresetId();
                if (preset_id < 31) {
                    preset_id++;
                } else {
                    preset_id = 0; // 最初のプリセットへ
                }
                synth.reset(); // ノートをリセット
                synth.loadPreset(preset_id);
                needsFullRedraw = true;
                manager->invalidate();
            }
            else if (cursor == C_ALGO) {
                // アルゴリズム変更（次へ）
                uint8_t algo_id = synth.getCurrentAlgorithmId();
                if (algo_id < 31) {
                    algo_id++;
                } else {
                    algo_id = 0; // 最初のアルゴリズムへ
                }
                synth.reset(); // ノートをリセット
                synth.setAlgorithm(algo_id);
                needsFullRedraw = true;
                manager->invalidate();
            }
        }

        // ENTERボタン：決定/実行
        else if (button == BTN_ET) {
            if (cursor == C_POLY) {
                // 緊急リセット
                synth.reset();
                manager->invalidate();
            }
            else if (cursor == C_FX) {
                // エフェクト設定画面へ
                manager->pushScreen(new FXScreen());
                return;
            }
            else if (cursor == C_MASTER) {
                // マスター設定画面へ
                manager->pushScreen(new MasterScreen());
                return;
            }
            else if (cursor >= C_OP1 && cursor <= C_OP6) {
                // オペレーター設定画面へ
                uint8_t opIndex = cursor - C_OP1;
                manager->pushScreen(new OperatorScreen(opIndex));
                return;
            }
            // TODO: 以下は後で実装
            // else if (cursor == C_MENU) {
            //     // メニュー画面へ
            // }
        }

        // CANCELボタン
        else if (button == BTN_CXL) {
            cursor = C_PRESET;
        }

        if (moved) {
            manager->invalidate();
        }
    }

    void draw(GFXcanvas16& canvas) override {
        static bool firstDraw = true;
        static int8_t lastCursor = -1;

        // onEnter後の初回描画フラグをリセット
        if (needsFullRedraw) {
            firstDraw = true;
            lastCursor = -1;
            needsFullRedraw = false;
        }

        // --- 1. 初回描画 (全画面) ---
        if (firstDraw) {
            canvas.fillScreen(Color::BLACK);

            // 各パーツを描画
            drawPresetHeader(canvas);

            // 装飾線と白帯
            canvas.drawFastHLine(0, 14, SCREEN_WIDTH, Color::WHITE);
            canvas.fillRect(0, 15, SCREEN_WIDTH, 10, Color::WHITE);

            drawAlgorithmLabel(canvas, (cursor == C_ALGO));
            canvas.drawFastHLine(0, 25, SCREEN_WIDTH, Color::WHITE);

            // オペレーター図形
            drawAlgoDiagram(canvas); // 内部で cursor を見てハイライト判定

            // Mボタン（マスター設定）
            drawMasterButton(canvas, (cursor == C_MASTER));

            // フッター
            canvas.drawFastHLine(0, FOOTER_Y, SCREEN_WIDTH, Color::WHITE);
            drawFooterItems(canvas); // FX, MENU, POLY

            firstDraw = false;
            lastCursor = cursor;
            manager->triggerFullTransfer();
        }

        // --- 2. カーソル移動時の部分更新 ---
        if (cursor != lastCursor) {
            // 移動元(previous) を「非選択」で再描画
            updateCursorElement(canvas, lastCursor);

            // 移動先(current) を「選択」で再描画
            updateCursorElement(canvas, cursor);

            lastCursor = cursor;
        }

        // --- 3. 定期更新 (POLY数) ---
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
        bool isSelected = (cursor == cursorPos);

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
        else if (cursorPos == C_MASTER) {
            // アルゴリズム図の左下に「M」を配置
            drawMasterButton(canvas, isSelected);
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
        Synth& synth = Synth::getInstance();
        canvas.fillRect(0, 0, SCREEN_WIDTH, 14, Color::BLACK);

        canvas.setTextSize(1);
        canvas.setTextColor(Color::WHITE);
        canvas.setCursor(2, 4);

        // 実際のSynth状態から表示
        char headerStr[32];
        sprintf(headerStr, "%03d:%s",
                synth.getCurrentPresetId() + 1,  // 1-indexed表示
                synth.getCurrentPresetName());
        canvas.print(headerStr);

        // 部分転送予約
        manager->transferPartial(0, 0, SCREEN_WIDTH, 14);
    }

    void drawAlgorithmLabel(GFXcanvas16& canvas, bool selected) {
        Synth& synth = Synth::getInstance();
        char algoStr[20];
        sprintf(algoStr, "Algorithm:%d", synth.getCurrentAlgorithmId() + 1); // 1-indexed

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
        Synth& synth = Synth::getInstance();
        // 座標計算（drawAlgoDiagramと同じロジック）
        // 実際のアルゴリズムIDを使用
        const Algorithm& algo = Algorithms::get(synth.getCurrentAlgorithmId());

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
        bool isEnabled = synth.getOperatorOsc(opIndex).isEnabled();

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
        Synth& synth = Synth::getInstance();
        int16_t w = str.length() * 6;
        int16_t h = 8;
        // 背景クリア範囲
        int16_t bgX = x - 1;
        int16_t bgY = y - 1;
        int16_t bgW = w + 2;
        int16_t bgH = h + 2;

        // FXの場合、有効なエフェクトがあるかチェック
        bool hasFx = (str == "FX") &&
                     (synth.isDelayEnabled() ||
                      synth.isLpfEnabled() ||
                      synth.isHpfEnabled());

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
        drawFooterItem(canvas, "FX", 10, FOOTER_Y + 5, (cursor == C_FX));

        // 区切り線
        canvas.drawFastVLine(30, FOOTER_Y + 1, 14, Color::MD_GRAY);

        // MENU
        String menuStr = "MENU";
        int16_t menuWidth = menuStr.length() * 6;
        drawFooterItem(canvas, menuStr, SCREEN_WIDTH - menuWidth - 4, FOOTER_Y + 5, (cursor == C_MENU));

        // 区切り線
        canvas.drawFastVLine(SCREEN_WIDTH - 34, FOOTER_Y + 1, 14, Color::MD_GRAY);

        // POLY
        updatePolyDisplay(canvas, lastPolyCount);
    }

    /**
     * @brief Mボタン（マスター設定）をアルゴリズム図の左下に描画
     */
    void drawMasterButton(GFXcanvas16& canvas, bool selected) {
        // 左下の空きスペース (FOOTER_Yより少し上、左端)
        int16_t x = 4;
        int16_t y = FOOTER_Y - 18;  // フッターより18px上
        int16_t size = 14;

        canvas.fillRect(x, y, size, size, Color::BLACK);
        
        if (selected) {
            canvas.fillRect(x, y, size, size, Color::WHITE);
            canvas.setTextColor(Color::BLACK);
        } else {
            canvas.drawRect(x, y, size, size, Color::MD_GRAY);
            canvas.setTextColor(Color::MD_GRAY);
        }
        
        canvas.setCursor(x + 4, y + 3);
        canvas.print("M");

        manager->transferPartial(x, y, size, size);
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
        bool isSelected = (cursor == C_POLY);

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
        Synth& synth = Synth::getInstance();
        // 実際のアルゴリズムIDを使用
        const Algorithm& algo = Algorithms::get(synth.getCurrentAlgorithmId());

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
                    int16_t srcCol = algo.positions[src].col;
                    int16_t srcRow = algo.positions[src].row;
                    int16_t dstCol = algo.positions[dst].col;
                    int16_t dstRow = algo.positions[dst].row;

                    // オペレーターの中心座標
                    int16_t x1 = originX + srcCol * GRID_W + OP_SIZE / 2;
                    int16_t y1 = originY + srcRow * GRID_H + OP_SIZE / 2;
                    int16_t x2 = originX + dstCol * GRID_W + OP_SIZE / 2;
                    int16_t y2 = originY + dstRow * GRID_H + OP_SIZE / 2;

                    // 同じ列または同じ行なら直線で接続
                    if (srcCol == dstCol || srcRow == dstRow) {
                        canvas.drawLine(x1, y1, x2, y2, lineColor);
                    } else {
                        // 異なる行・列の場合、L字型を試みる
                        // 折れ曲がりポイントで他のオペレーターと重なるかチェック

                        // 方式: srcの下端から縦に降りて、dstの高さで横に曲がる
                        // 折れ曲がりY座標 = dstの上端より少し上
                        int16_t bendY = originY + dstRow * GRID_H - (GRID_H - OP_SIZE) / 2;

                        // 折れ曲がりの水平線上（bendYの行）に他のオペレーターがあるかチェック
                        bool hasCollision = false;
                        int16_t minCol = (srcCol < dstCol) ? srcCol : dstCol;
                        int16_t maxCol = (srcCol > dstCol) ? srcCol : dstCol;

                        for (int i = 0; i < MAX_OPERATORS; i++) {
                            if (i == src || i == dst) continue;

                            int16_t opCol = algo.positions[i].col;
                            int16_t opRow = algo.positions[i].row;

                            // 水平線が通る範囲内にオペレーターがあるかチェック
                            // オペレーターのY範囲と折れ曲がりY座標が重なるか
                            int16_t opTop = originY + opRow * GRID_H;
                            int16_t opBottom = opTop + OP_SIZE;

                            // X方向が範囲内かつY方向で重なる場合
                            if (opCol > minCol && opCol < maxCol) {
                                if (bendY >= opTop && bendY <= opBottom) {
                                    hasCollision = true;
                                    break;
                                }
                            }
                        }

                        if (!hasCollision) {
                            // L字型で描画（縦→横）
                            canvas.drawLine(x1, y1, x1, bendY, lineColor);  // 縦線
                            canvas.drawLine(x1, bendY, x2, bendY, lineColor); // 横線
                            canvas.drawLine(x2, bendY, x2, y2, lineColor);  // 縦線
                        } else {
                            // 衝突がある場合は斜め線
                            canvas.drawLine(x1, y1, x2, y2, lineColor);
                        }
                    }
                }
            }
        }

        // --- B. フィードバックループの描画 ---
        if (algo.feedback_op >= 0 && algo.feedback_op < MAX_OPERATORS) {
            int dstOp = algo.feedback_op;  // フィードバック先のオペレーター

            // mod_maskからフィードバック元のオペレーターを特定
            int srcOp = -1;
            uint8_t mask = algo.mod_mask[dstOp];
            for (int i = 0; i < MAX_OPERATORS; i++) {
                if (mask & (1 << i)) {
                    srcOp = i;
                    break;  // 最初に見つかったものを使用
                }
            }

            // srcOpが見つからない場合は自己フィードバック
            if (srcOp < 0) {
                srcOp = dstOp;
            }

            // 送信元と送信先の座標を取得
            int16_t srcX = originX + algo.positions[srcOp].col * GRID_W;
            int16_t srcY = originY + algo.positions[srcOp].row * GRID_H;
            int16_t dstX = originX + algo.positions[dstOp].col * GRID_W;
            int16_t dstY = originY + algo.positions[dstOp].row * GRID_H;

            // コの字型のループ線の調整パラメータ
            int16_t loopOffsetRight = 3;  // 右へのはみ出し量
            int16_t loopOffsetTop = 4;    // 上へのはみ出し量

            // 送信元オペレーターの右側中央から線を開始
            int16_t startX = srcX + OP_SIZE;
            int16_t startY = srcY + OP_SIZE / 2;

            // 送信先オペレーターの上側中央に線を接続
            int16_t endX = dstX + OP_SIZE / 2;
            int16_t endY = dstY;

            // 右側と上側の折り返しポイント
            int16_t rightX = srcX + OP_SIZE + loopOffsetRight;
            int16_t topY = dstY - loopOffsetTop;

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
            bool isSel = (cursor == (C_OP1 + i));
            drawOpBox(canvas, i, isSel);
        }
    }
};

