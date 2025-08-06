#pragma once

#include <array>
#include <cstdint>

namespace puyo {

// ゲーム定数
static constexpr int FIELD_WIDTH = 6;   // フィールド幅
static constexpr int FIELD_HEIGHT = 14; // フィールド高さ
static constexpr int VISIBLE_HEIGHT = 12; // 可視部分の高さ
static constexpr int HIDDEN_HEIGHT = 2;   // 隠し段の高さ
static constexpr int FIELD_SIZE = FIELD_WIDTH * FIELD_HEIGHT; // 84

// ぷよの色定義
enum class PuyoColor : uint8_t {
    EMPTY = 0,    // 空
    RED = 1,      // 赤
    GREEN = 2,    // 緑
    BLUE = 3,     // 青
    YELLOW = 4,   // 黄
    PURPLE = 5,   // 紫
    GARBAGE = 6   // おじゃま
};

// 色の数（EMPTYを除く）
static constexpr int COLOR_COUNT = 6;

// 回転状態
enum class Rotation : uint8_t {
    UP = 0,       // 上（初期状態）
    RIGHT = 1,    // 右
    DOWN = 2,     // 下
    LEFT = 3      // 左
};

// フィールド位置
struct Position {
    int x;  // 列 (0-5)
    int y;  // 段 (0-13, 0が最下段)
    
    Position(int x = 0, int y = 0) : x(x), y(y) {}
    
    bool is_valid() const {
        return x >= 0 && x < FIELD_WIDTH && y >= 0 && y < FIELD_HEIGHT;
    }
    
    // ビット位置を計算（y * FIELD_WIDTH + x）
    int to_bit_index() const {
        return y * FIELD_WIDTH + x;
    }
};

// 128ビットのビットボード
using BitBoard128 = __uint128_t;

// ビットボード操作関数
inline void set_bit(BitBoard128& board, int index) {
    board |= (static_cast<BitBoard128>(1) << index);
}

inline void clear_bit(BitBoard128& board, int index) {
    board &= ~(static_cast<BitBoard128>(1) << index);
}

inline bool get_bit(const BitBoard128& board, int index) {
    return (board & (static_cast<BitBoard128>(1) << index)) != 0;
}

inline bool is_empty(const BitBoard128& board) {
    return board == 0;
}

// 操作ぷよ
struct PuyoPair {
    PuyoColor axis;   // 軸ぷよ
    PuyoColor child;  // 子ぷよ
    Position pos;     // 軸ぷよの位置
    Rotation rot;     // 回転状態
    
    PuyoPair(PuyoColor axis = PuyoColor::EMPTY, 
             PuyoColor child = PuyoColor::EMPTY,
             Position pos = Position(2, 11),  // デフォルト位置（3列目12段目）
             Rotation rot = Rotation::UP)
        : axis(axis), child(child), pos(pos), rot(rot) {}
    
    // 子ぷよの位置を取得
    Position get_child_position() const;
};

// フィールド状態（色ごとのビットマップ）
struct FieldBitBoards {
    std::array<BitBoard128, COLOR_COUNT> color_bits;  // 各色のビットマップ
    
    // コンストラクタ
    FieldBitBoards() {
        for (auto& bits : color_bits) {
            bits = 0;
        }
    }
    
    // 指定位置の色を設定
    void set_color(const Position& pos, PuyoColor color);
    
    // 指定位置の色を取得
    PuyoColor get_color(const Position& pos) const;
    
    // 指定位置をクリア
    void clear_position(const Position& pos);
    
    // フィールド全体をクリア
    void clear_all();
    
    // 指定色のビットマップを取得
    const BitBoard128& get_color_bits(PuyoColor color) const;
    
    // 空のセルのビットマップを取得
    BitBoard128 get_empty_bits() const;
};

} // namespace puyo