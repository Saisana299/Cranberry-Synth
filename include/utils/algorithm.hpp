#pragma once

#include <cstdint>

constexpr uint8_t MAX_OPERATORS = 6;

// 描画用の座標構造体 (メモリ節約のため int8_t)
struct OpCoord {
    int8_t col; // 列番号 (グリッドX)
    int8_t row; // 行番号 (グリッドY)
};

// アルゴリズムデータ構造
struct Algorithm {
    // オペレーターの計算順序 (モジュレーター→キャリアの順)
    uint8_t exec_order[MAX_OPERATORS];

    // 各オペレーターへの入力元を表すビットマスク
    // mod_mask[0] の 1ビット目が立っていたら、Op0はOp1からの入力を受け取る
    uint8_t mod_mask[MAX_OPERATORS];

    // 最終出力（キャリア）となるオペレーターのビットマスク
    // ビット0が立っていたらOp0はキャリアとして音声出力される
    uint8_t output_mask;

    // フィードバックを行うオペレーターの番号 (0-5)。 ない場合は -1
    // このオペレーターは、mod_maskで指定された入力元からフィードバックを受ける
    // 例: feedback_op=5, mod_mask[5]=(1<<3) → Op5はOp3からフィードバック
    // 自己フィードバックの場合: feedback_op=5, mod_mask[5]=0 → Op5は自己フィードバック
    int8_t feedback_op;

    // UI用 各オペレーターのグリッド位置（列、行）
    OpCoord positions[MAX_OPERATORS];
};

// 全アルゴリズムを管理
class Algorithms {
public:
    static const Algorithm& get(uint8_t id) {
        // 範囲外アクセス防止
        if (id >= 32) id = 0;
        return algorithms_[id];
    }

private:
    // ヘッダ内で実体を定義するために inline を使用 (C++17~)
    static inline const Algorithm algorithms_[32] = {

        // --- No.1 ---
        // [1]->[0], [5*]->[4]->[3]->[2]
        {
            // exec_order
            {5, 4, 3, 1, 0, 2}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                0,      // Op1
                (1<<3), // Op2 <- Op3
                (1<<4), // Op3 <- Op4
                (1<<5), // Op4 <- Op5
                0       // Op5
            },

            // output_mask
            (1<<0) | (1<<2), // Out: 0, 2

            // feedback_op
            5,

            // positions
            {
                {0, 2}, // Op0
                {0, 1}, // Op1
                {1, 2}, // Op2
                {1, 1}, // Op3
                {2, 1}, // Op4
                {2, 0}, // Op5
            }
        },

        // --- No.2 ---
        // [1*]->[0], [5]->[4]->[3]->[2]
        {
            // exec_order
            {5, 4, 3, 1, 0, 2}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                0,      // Op1
                (1<<3), // Op2 <- Op3
                (1<<4), // Op3 <- Op4
                (1<<5), // Op4 <- Op5
                0       // Op5
            },

            // output_mask
            (1<<0) | (1<<2), // Out: 0, 2

            // feedback_op
            1,

            // positions
            {
                {0, 2}, // Op0
                {0, 1}, // Op1
                {1, 2}, // Op2
                {1, 1}, // Op3
                {2, 1}, // Op4
                {2, 0}, // Op5
            }
        },

        // --- No.3 ---
        // [2]->[1]->[0], [5*]->[4]->[3]
        {
            // exec_order
            {2, 5, 1, 4, 0, 3}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                (1<<2), // Op1 <- Op2
                0,      // Op2
                (1<<4), // Op3 <- Op4
                (1<<5), // Op4 <- Op5
                0       // Op5
            },

            // output_mask
            (1<<0) | (1<<3), // Out: 0, 3

            // feedback_op
            5,

            // positions
            {
                {0, 2}, // Op0
                {0, 1}, // Op1
                {0, 0}, // Op2
                {1, 2}, // Op3
                {1, 1}, // Op4
                {1, 0}, // Op5
            }
        },

        // --- No.4 ---
        // [2]->[1]->[0], [5]->[4]->[3], [3]->[5]
        {
            // exec_order
            {2, 5, 1, 4, 0, 3}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                (1<<2), // Op1 <- Op2
                0,      // Op2
                (1<<4), // Op3 <- Op4
                (1<<5), // Op4 <- Op5
                (1<<3), // Op5 <- Op3
            },

            // output_mask
            (1<<0) | (1<<3), // Out: 0, 3

            // feedback_op
            5,

            // positions
            {
                {0, 2}, // Op0
                {0, 1}, // Op1
                {0, 0}, // Op2
                {1, 2}, // Op3
                {1, 1}, // Op4
                {1, 0}, // Op5
            }
        },

        // --- No.5 ---
        // [1]->[0], [3]->[2], [5*]->[4]
        {
            // exec_order
            {1, 3, 5, 0, 2, 4}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                0,      // Op1
                (1<<3), // Op2 <- Op3
                0,      // Op3
                (1<<5), // Op4 <- Op5
                0,      // Op5
            },

            // output_mask
            (1<<0) | (1<<2) | (1<<4), // Out: 0, 2, 4

            // feedback_op
            5,

            // positions
            {
                {0, 1}, // Op0
                {0, 0}, // Op1
                {1, 1}, // Op2
                {1, 0}, // Op3
                {2, 1}, // Op4
                {2, 0}, // Op5
            }
        },

        // --- No.6 ---
        // [1]->[0], [3]->[2], [5]->[4], [4]->[5]
        {
            // exec_order
            {1, 3, 5, 0, 2, 4}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                0,      // Op1
                (1<<3), // Op2 <- Op3
                0,      // Op3
                (1<<5), // Op4 <- Op5
                (1<<4), // Op5 <- Op4
            },

            // output_mask
            (1<<0) | (1<<2) | (1<<4), // Out: 0, 2, 4

            // feedback_op
            5,

            // positions
            {
                {0, 1}, // Op0
                {0, 0}, // Op1
                {1, 1}, // Op2
                {1, 0}, // Op3
                {2, 1}, // Op4
                {2, 0}, // Op5
            }
        },

        // --- No.7 ---
        // [1]->[0], [3]->[2], [5*]->[4]->[2]
        {
            // exec_order
            {1, 3, 5, 4, 0, 2}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                0,      // Op1
                (1<<3) | (1<<4), // Op2 <- Op3, Op4
                0,      // Op3
                (1<<5), // Op4 <- Op5
                0, // Op5
            },

            // output_mask
            (1<<0) | (1<<2), // Out: 0, 2

            // feedback_op
            5,

            // positions
            {
                {0, 2}, // Op0
                {0, 1}, // Op1
                {1, 2}, // Op2
                {1, 1}, // Op3
                {2, 1}, // Op4
                {2, 0}, // Op5
            }
        },

        // --- No.8 ---
        // [1]->[0], [3*]->[2], [5]->[4]->[2]
        {
            // exec_order
            {1, 3, 5, 4, 0, 2}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                0,      // Op1
                (1<<3) | (1<<4), // Op2 <- Op3, Op4
                0,      // Op3
                (1<<5), // Op4 <- Op5
                0, // Op5
            },

            // output_mask
            (1<<0) | (1<<2), // Out: 0, 2

            // feedback_op
            3,

            // positions
            {
                {0, 2}, // Op0
                {0, 1}, // Op1
                {1, 2}, // Op2
                {1, 1}, // Op3
                {2, 1}, // Op4
                {2, 0}, // Op5
            }
        },

        // --- No.9 ---
        // [1*]->[0], [3]->[2], [5]->[4]->[2]
        {
            // exec_order
            {1, 3, 5, 4, 0, 2}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                0,      // Op1
                (1<<3) | (1<<4), // Op2 <- Op3, Op4
                0,      // Op3
                (1<<5), // Op4 <- Op5
                0, // Op5
            },

            // output_mask
            (1<<0) | (1<<2), // Out: 0, 2

            // feedback_op
            1,

            // positions
            {
                {0, 2}, // Op0
                {0, 1}, // Op1
                {1, 2}, // Op2
                {1, 1}, // Op3
                {2, 1}, // Op4
                {2, 0}, // Op5
            }
        },

        // --- No.10 ---
        // [4]->[3], [5]->[3], [2*]->[1]->[0]
        {
            // exec_order
            {4, 5, 2, 1, 3, 0}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                (1<<2), // Op1 <- Op2
                0,      // Op2
                (1<<4) | (1<<5),  // Op3 <- Op4, Op5
                0,      // Op4
                0,      // Op5
            },

            // output_mask
            (1<<0) | (1<<3), // Out: 0, 3

            // feedback_op
            2,

            // positions
            {
                {2, 2}, // Op0
                {2, 1}, // Op1
                {2, 0}, // Op2
                {1, 2}, // Op3
                {0, 1}, // Op4
                {1, 1}, // Op5
            }
        },

        // --- No.11 ---
        // [4]->[3], [5*]->[3], [2]->[1]->[0]
        {
            // exec_order
            {4, 5, 2, 1, 3, 0}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                (1<<2), // Op1 <- Op2
                0,      // Op2
                (1<<4) | (1<<5),  // Op3 <- Op4, Op5
                0,      // Op4
                0,      // Op5
            },

            // output_mask
            (1<<0) | (1<<3), // Out: 0, 3

            // feedback_op
            5,

            // positions
            {
                {2, 2}, // Op0
                {2, 1}, // Op1
                {2, 0}, // Op2
                {1, 2}, // Op3
                {0, 1}, // Op4
                {1, 1}, // Op5
            }
        },

        // --- No.12 ---
        // [3]->[2], [4]->[2], [5]->[2], [1*]->[0]
        {
            // exec_order
            {3, 4, 5, 1, 2, 0}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                0,      // Op1
                (1<<3) | (1<<4) | (1<<5), // Op2 <- Op3, Op4, Op5
                0,      // Op3
                0,      // Op4
                0,      // Op5
            },

            // output_mask
            (1<<0) | (1<<2), // Out: 0, 2

            // feedback_op
            1,

            // positions
            {
                {3, 1}, // Op0
                {3, 0}, // Op1
                {1, 1}, // Op2
                {0, 0}, // Op3
                {1, 0}, // Op4
                {2, 0}, // Op5
            }
        },

        // --- No.13 ---
        // [3]->[2], [4]->[2], [5*]->[2], [1]->[0]
        {
            // exec_order
            {3, 4, 5, 1, 2, 0}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                0,      // Op1
                (1<<3) | (1<<4) | (1<<5), // Op2 <- Op3, Op4, Op5
                0,      // Op3
                0,      // Op4
                0,      // Op5
            },

            // output_mask
            (1<<0) | (1<<2), // Out: 0, 2

            // feedback_op
            5,

            // positions
            {
                {3, 1}, // Op0
                {3, 0}, // Op1
                {1, 1}, // Op2
                {0, 0}, // Op3
                {1, 0}, // Op4
                {2, 0}, // Op5
            }
        },

        // --- No.14 ---
        // [1]->[0], [4]->[3]->[2], [5*]->[3]
        {
            // exec_order
            {1, 4, 5, 3, 0, 2}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                0,      // Op1
                (1<<3) | (1<<5), // Op2 <- Op3, Op5
                (1<<4), // Op3 <- Op4
                0,      // Op4
                0,      // Op5
            },

            // output_mask
            (1<<0) | (1<<2), // Out: 0, 2

            // feedback_op
            5,

            // positions
            {
                {0, 2}, // Op0
                {0, 1}, // Op1
                {1, 2}, // Op2
                {1, 1}, // Op3
                {1, 0}, // Op4
                {2, 0}, // Op5
            }
        },

        // --- No.15 ---
        // [1*]->[0], [4]->[3]->[2], [5]->[3]
        {
            // exec_order
            {1, 4, 5, 3, 0, 2}, // 計算順: Mod -> Car

            // mod_mask
            {
                (1<<1), // Op0 <- Op1
                0,      // Op1
                (1<<3) | (1<<5), // Op2 <- Op3, Op5
                (1<<4), // Op3 <- Op4
                0,      // Op4
                0,      // Op5
            },

            // output_mask
            (1<<0) | (1<<2), // Out: 0, 2

            // feedback_op
            1,

            // positions
            {
                {0, 2}, // Op0
                {0, 1}, // Op1
                {1, 2}, // Op2
                {1, 1}, // Op3
                {1, 0}, // Op4
                {2, 0}, // Op5
            }
        },
    };
};