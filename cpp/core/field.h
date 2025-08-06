#pragma once

#include "puyo_types.h"
#include <vector>
#include <string>

namespace puyo {

class Field {
private:
    FieldBitBoards field_bits_;
    std::array<bool, FIELD_WIDTH> row14_used_;  // 14段目使用フラグ（列ごと）
    
public:
    Field();
    
    // フィールド操作
    void clear();
    PuyoColor get_puyo(const Position& pos) const;
    void set_puyo(const Position& pos, PuyoColor color);
    void remove_puyo(const Position& pos);
    
    // 14段目特殊仕様
    bool can_place_at_row14(int column) const;
    void mark_row14_used(int column);
    bool is_row14_used(int column) const;
    
    // 設置可能性判定
    bool can_place_puyo_pair(const PuyoPair& pair) const;
    
    // ぷよ設置
    bool place_puyo_pair(const PuyoPair& pair);
    
    // 落下処理
    bool apply_gravity();
    
    // フィールド状態取得
    const FieldBitBoards& get_field_bits() const { return field_bits_; }
    
    // 敗北判定（窒息点チェック）
    bool is_game_over() const;
    
    // デバッグ用：フィールド状態を文字列で取得
    std::string to_string() const;
    
    // コピー
    Field(const Field& other) = default;
    Field& operator=(const Field& other) = default;
};

} // namespace puyo