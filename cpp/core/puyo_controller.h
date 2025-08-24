#pragma once

#include "puyo_types.h"
#include "field.h"

namespace puyo {

// 操作コマンド
enum class MoveCommand {
    LEFT,          // 左移動
    RIGHT,         // 右移動
    ROTATE_CW,     // 時計回り回転
    ROTATE_CCW,    // 反時計回り回転
    DROP,          // 下移動/ドロップ
    NONE           // 何もしない
};

class PuyoController {
private:
    Field* field_;
    PuyoPair current_pair_;
    bool next_rotation_is_quick_turn_;
    
public:
    PuyoController(Field* field);
    
    // 操作ぷよの設定
    void set_current_pair(const PuyoPair& pair);
    const PuyoPair& get_current_pair() const { return current_pair_; }
    
    // 操作実行
    bool execute_command(MoveCommand command);
    
    // 移動系操作
    bool move_left();
    bool move_right();
    bool move_down();
    
    // 回転操作
    bool rotate_clockwise();
    bool rotate_counter_clockwise();
    
    // 配置操作
    bool place_current_pair();
    
    // 操作可能性判定
    bool can_move_left() const;
    bool can_move_right() const;
    bool can_move_down() const;
    bool can_rotate_clockwise() const;
    bool can_rotate_counter_clockwise() const;
    
    // ユーティリティ
    PuyoPair create_rotated_pair(const PuyoPair& pair, bool clockwise) const;
    bool is_valid_position(const PuyoPair& pair) const;
    
private:
    // 回転処理（キック・クイックターン対応）
    bool perform_rotation(bool clockwise);
    bool can_perform_quick_turn() const;
    bool perform_quick_turn();
};

} // namespace puyo