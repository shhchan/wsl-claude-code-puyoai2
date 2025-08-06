#include "puyo_types.h"

namespace puyo {

// PuyoPairの子ぷよ位置計算
Position PuyoPair::get_child_position() const {
    Position child_pos = pos;
    
    switch (rot) {
        case Rotation::UP:
            child_pos.y += 1;  // 軸ぷよの上
            break;
        case Rotation::RIGHT:
            child_pos.x += 1;  // 軸ぷよの右
            break;
        case Rotation::DOWN:
            child_pos.y -= 1;  // 軸ぷよの下
            break;
        case Rotation::LEFT:
            child_pos.x -= 1;  // 軸ぷよの左
            break;
    }
    
    return child_pos;
}

// FieldBitBoardsのメソッド実装
void FieldBitBoards::set_color(const Position& pos, PuyoColor color) {
    if (!pos.is_valid()) return;
    
    int bit_index = pos.to_bit_index();
    
    // まず該当位置をすべてクリア
    clear_position(pos);
    
    // 指定色のビットを設定（EMPTYの場合は何もしない）
    if (color != PuyoColor::EMPTY && static_cast<int>(color) <= COLOR_COUNT) {
        int color_index = static_cast<int>(color) - 1;  // 1-indexedを0-indexedに変換
        set_bit(color_bits[color_index], bit_index);
    }
}

PuyoColor FieldBitBoards::get_color(const Position& pos) const {
    if (!pos.is_valid()) return PuyoColor::EMPTY;
    
    int bit_index = pos.to_bit_index();
    
    // 各色のビットを確認
    for (int i = 0; i < COLOR_COUNT; ++i) {
        if (get_bit(color_bits[i], bit_index)) {
            return static_cast<PuyoColor>(i + 1);  // 0-indexedを1-indexedに変換
        }
    }
    
    return PuyoColor::EMPTY;
}

void FieldBitBoards::clear_position(const Position& pos) {
    if (!pos.is_valid()) return;
    
    int bit_index = pos.to_bit_index();
    
    // すべての色のビットマップから該当位置をクリア
    for (auto& bits : color_bits) {
        clear_bit(bits, bit_index);
    }
}

void FieldBitBoards::clear_all() {
    for (auto& bits : color_bits) {
        bits = 0;
    }
}

const BitBoard128& FieldBitBoards::get_color_bits(PuyoColor color) const {
    static BitBoard128 empty_board = 0;
    
    if (color == PuyoColor::EMPTY || static_cast<int>(color) > COLOR_COUNT) {
        return empty_board;
    }
    
    int color_index = static_cast<int>(color) - 1;
    return color_bits[color_index];
}

BitBoard128 FieldBitBoards::get_empty_bits() const {
    BitBoard128 occupied = 0;
    
    // すべての色のビットをOR
    for (const auto& bits : color_bits) {
        occupied |= bits;
    }
    
    // 有効なフィールド範囲のマスク（84ビット分）
    BitBoard128 field_mask = (static_cast<BitBoard128>(1) << FIELD_SIZE) - 1;
    
    // 空のセルは occupied の逆
    return (~occupied) & field_mask;
}

} // namespace puyo