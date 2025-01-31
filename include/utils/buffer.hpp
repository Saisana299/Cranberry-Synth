#pragma once

#include <Arduino.h>

#define NEXT_IDX(idx) (((idx) + 1) % (RB_SIZE * 2))

template <typename T, uint16_t RB_SIZE>
class RingBuffer {
private:
    uint32_t read_idx = 0;
    uint32_t write_idx = 0;
    T buff[RB_SIZE];  // 要素の型 T に対応したバッファ

public:
    RingBuffer() {
        reset();
    }

    void reset() {
        write_idx = 0;
        read_idx = 0;

        // バッファを初期化
        for (uint16_t i = 0; i < RB_SIZE; ++i) {
            buff[i] = T();  // 型Tの初期値で埋める
        }
    }

    bool write(T in) {
        if (write_idx > read_idx ? write_idx - read_idx == RB_SIZE
                                 : read_idx - write_idx == RB_SIZE) {
            // バッファが満杯の場合
            return false;
        }
        buff[write_idx] = in;
        write_idx = NEXT_IDX(write_idx);
        return true;
    }

    bool read(T *out) {
        if(read_idx == write_idx) {
            // バッファが空の場合
            return false;
        }
        *out = buff[read_idx];
        read_idx = NEXT_IDX(read_idx);
        return true;
    }
};

template <typename T, uint16_t RB_SIZE>
class IntervalRingBuffer {
private:
    int32_t read_idx = 0;
    int32_t write_idx = 0;
    T buff[RB_SIZE];  // 要素の型 T に対応したバッファ

public:
    IntervalRingBuffer() {
        reset();
    }

    void reset() {
        write_idx = 0;
        read_idx = RB_SIZE / 2;

        // バッファを初期化
        for (uint16_t i = 0; i < RB_SIZE; ++i) {
            buff[i] = T();  // 型Tの初期値で埋める
        }
    }

    void setInterval(int32_t interval) {
        interval = interval % RB_SIZE;
        if(interval <= 0) {
            interval = 1;
        }
        write_idx = (read_idx + interval) % RB_SIZE;
    }

    void write(T in) {
        buff[write_idx] = in;
    }

    T read(int32_t index = 0) {
        int32_t tmp = read_idx + index;
        while(tmp < 0) {
            tmp += RB_SIZE;
        }
        tmp = tmp % RB_SIZE;

        return buff[tmp];
    }

    void update() {
        read_idx = (read_idx + 1) % RB_SIZE;
        write_idx = (write_idx + 1) % RB_SIZE;
    }
};