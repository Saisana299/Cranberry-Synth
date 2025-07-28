#pragma once

#include <cstdint>

// オペレーターの接続先を定義（-1は変調入力なし）
struct OpConnection {
    int8_t input_op_index;
};

// アルゴリズムは6つのオペレーターの接続情報の集まり
struct Algorithm {
    OpConnection connections[6];
    bool is_carrier[6]; // 各オペレーターがキャリアかどうかのフラグ
};

// 全32種類のアルゴリズムを定義する静的配列
class Algorithms {
public:
    static const Algorithm& get(uint8_t algorithm_id) {
        // algorithm_idは0-31
        return all_algorithms_[algorithm_id];
    }
private:
    static const Algorithm all_algorithms_[32];//todo
};