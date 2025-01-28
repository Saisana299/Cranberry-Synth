#pragma once

/* Algorithms for 4 operators */

struct OperatorMap4 {
    OpMode modes[4];
    bool connections[4][4];
}

enum class OpMode {
    Carrier, Modulator
};

class Algorithms4 {
public:
    static constexpr OperatorMap4 maps = {
        // [OP4] <-- [OP4]
        //   ↓
        // [OP3]
        //   ↓
        // [OP2]
        //   ↓
        // [OP1]
        //   ↓
        // 出力音
        {
            {OpMode::Carrier, OpMode::Modulator, OpMode::Modulator, OpMode::Modulator},
            {
                {0, 0, 0, 0},
                {1, 0, 0, 0},
                {0, 1, 0, 0},
                {0, 0, 1, 1}
            }
        },

        // [OP4] <-- [OP4]
        //   ↓
        // [OP2] <-- [OP3]
        //   ↓
        // [OP1]
        //   ↓
        // 出力音
        {
            {OpMode::Carrier, OpMode::Modulator, OpMode::Modulator, OpMode::Modulator},
            {
                {0, 0, 0, 0},
                {1, 0, 0, 0},
                {0, 1, 0, 0},
                {0, 1, 0, 1}
            }
        },

        // [OP2] <-- [OP2]
        //   ↓
        // [OP1] <-- [OP3] <-- [OP4]
        //   ↓
        // 出力音
        {
            {OpMode::Carrier, OpMode::Modulator, OpMode::Modulator, OpMode::Modulator},
            {
                {0, 0, 0, 0},
                {1, 1, 0, 0},
                {1, 0, 0, 0},
                {0, 0, 1, 0}
            }
        },

        // [OP4] <-- [OP4]
        //   ↓
        // [OP3]
        //   ↓
        // [OP1] <-- [OP2]
        //   ↓
        // 出力音
        {
            {OpMode::Carrier, OpMode::Modulator, OpMode::Modulator, OpMode::Modulator},
            {
                {0, 0, 0, 0},
                {1, 0, 0, 0},
                {1, 0, 0, 0},
                {0, 0, 1, 1}
            }
        },

        // [OP4] <-- [OP4]   [OP2]
        //   ↓                 ↓
        // [OP3]             [OP1]
        //   ↓                 ↓
        // 出力音             出力音
        {
            {OpMode::Carrier, OpMode::Modulator, OpMode::Carrier, OpMode::Modulator},
            {
                {0, 0, 0, 0},
                {1, 0, 0, 0},
                {0, 0, 0, 0},
                {0, 0, 1, 1}
            }
        },

        //      [OP4] <-- [OP4]
        //        ↓
        // [OP1, OP2, OP3]
        //        ↓
        //      出力音
        {
            {OpMode::Carrier, OpMode::Carrier, OpMode::Carrier, OpMode::Modulator},
            {
                {0, 0, 0, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0},
                {1, 1, 1, 1}
            }
        },

        //               [OP4] <-- [OP4]
        //                 ↓
        // [OP1]  [OP2]  [OP3]
        //   ↓      ↓      ↓
        //        出力音
        {
            {OpMode::Carrier, OpMode::Carrier, OpMode::Carrier, OpMode::Modulator},
            {
                {0, 0, 0, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 1}
            }
        },

        // [OP1]  [OP2]  [OP3]  [OP4] <-- [OP4]
        //   ↓      ↓      ↓      ↓
        //           出力音
        {
            {OpMode::Carrier, OpMode::Carrier, OpMode::Carrier, OpMode::Carrier},
            {
                {0, 0, 0, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 1}
            }
        },
    };
};