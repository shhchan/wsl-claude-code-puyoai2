#include "puyo_controller.h"

namespace puyo {

PuyoController::PuyoController(Field* field) : field_(field) {
    // デフォルトの空ペアで初期化
    current_pair_ = PuyoPair();
}

void PuyoController::set_current_pair(const PuyoPair& pair) {
    current_pair_ = pair;
}

bool PuyoController::execute_command(MoveCommand command) {
    switch (command) {
        case MoveCommand::LEFT:
            return move_left();
        case MoveCommand::RIGHT:
            return move_right();
        case MoveCommand::DROP:
            return move_down();
        case MoveCommand::ROTATE_CW:
            return rotate_clockwise();
        case MoveCommand::ROTATE_CCW:
            return rotate_counter_clockwise();
        case MoveCommand::NONE:
            return true;
        default:
            return false;
    }
}

bool PuyoController::move_left() {
    if (!can_move_left()) {
        return false;
    }
    
    current_pair_.pos.x--;
    return true;
}

bool PuyoController::move_right() {
    if (!can_move_right()) {
        return false;
    }
    
    current_pair_.pos.x++;
    return true;
}

bool PuyoController::move_down() {
    if (!can_move_down()) {
        return false;
    }
    
    current_pair_.pos.y--;
    return true;
}

bool PuyoController::rotate_clockwise() {
    return perform_rotation(true);
}

bool PuyoController::rotate_counter_clockwise() {
    return perform_rotation(false);
}

// 回転処理（キック・クイックターン対応）
bool PuyoController::perform_rotation(bool clockwise) {
    PuyoPair original = current_pair_;
    PuyoPair rotated = create_rotated_pair(current_pair_, clockwise);
    
    // 通常回転を試行
    if (is_valid_position(rotated)) {
        current_pair_ = rotated;
        return true;
    }
    
    // キック処理を試行
    Position child_pos = rotated.get_child_position();
    bool child_blocked = !child_pos.is_valid() || 
                        (field_ && field_->get_puyo(child_pos) != PuyoColor::EMPTY);
    
    if (child_blocked) {
        // 現在の回転状態と回転方向に基づいてキック方向を決定
        std::pair<int, int> kick_offset = {0, 0};
        
        if (clockwise) {
            switch (original.rot) {
                case Rotation::UP:    // 上から時計回り（右へ）
                    if (child_pos.x >= FIELD_WIDTH || 
                        (field_ && child_pos.x < FIELD_WIDTH && field_->get_puyo(child_pos) != PuyoColor::EMPTY)) {
                        kick_offset = {-1, 0};  // 右に壁・ぷよがある場合: x方向に-1
                    }
                    break;
                case Rotation::RIGHT: // 右から時計回り（下へ）
                    if (child_pos.y < 0 || 
                        (field_ && child_pos.y >= 0 && field_->get_puyo(child_pos) != PuyoColor::EMPTY)) {
                        kick_offset = {0, 1};   // 下に壁・ぷよがある場合: y方向に+1
                    }
                    break;
                case Rotation::DOWN:  // 下から時計回り（左へ）
                    if (child_pos.x < 0 || 
                        (field_ && child_pos.x >= 0 && field_->get_puyo(child_pos) != PuyoColor::EMPTY)) {
                        kick_offset = {1, 0};   // 左に壁・ぷよがある場合: x方向に+1
                    }
                    break;
                case Rotation::LEFT:  // 左から時計回り（上へ）
                    // 上への回転では通常キックは発生しない
                    break;
            }
        } else {
            switch (original.rot) {
                case Rotation::UP:    // 上から反時計回り（左へ）
                    if (child_pos.x < 0 || 
                        (field_ && child_pos.x >= 0 && field_->get_puyo(child_pos) != PuyoColor::EMPTY)) {
                        kick_offset = {1, 0};   // 左に壁・ぷよがある場合: x方向に+1
                    }
                    break;
                case Rotation::LEFT:  // 左から反時計回り（下へ）
                    if (child_pos.y < 0 || 
                        (field_ && child_pos.y >= 0 && field_->get_puyo(child_pos) != PuyoColor::EMPTY)) {
                        kick_offset = {0, 1};   // 下に壁・ぷよがある場合: y方向に+1
                    }
                    break;
                case Rotation::DOWN:  // 下から反時計回り（右へ）
                    if (child_pos.x >= FIELD_WIDTH || 
                        (field_ && child_pos.x < FIELD_WIDTH && field_->get_puyo(child_pos) != PuyoColor::EMPTY)) {
                        kick_offset = {-1, 0};  // 右に壁・ぷよがある場合: x方向に-1
                    }
                    break;
                case Rotation::RIGHT: // 右から反時計回り（上へ）
                    // 上への回転では通常キックは発生しない
                    break;
            }
        }
        
        // キックを試行
        if (kick_offset.first != 0 || kick_offset.second != 0) {
            PuyoPair kicked = rotated;
            kicked.pos.x += kick_offset.first;
            kicked.pos.y += kick_offset.second;
            
            if (is_valid_position(kicked)) {
                current_pair_ = kicked;
                return true;
            }
        }
    }
    
    // 左右両方に壁・ぷよがある場合: クイックターンへ
    if (can_perform_quick_turn()) {
        return perform_quick_turn();
    }
    
    // 回転失敗
    return false;
}

// クイックターン可能判定
bool PuyoController::can_perform_quick_turn() const {
    // 両側が壁または他のぷよで塞がれているかチェック
    Position left_pos(current_pair_.pos.x - 1, current_pair_.pos.y);
    Position right_pos(current_pair_.pos.x + 1, current_pair_.pos.y);
    
    bool left_blocked = !left_pos.is_valid() || 
                       (field_ && field_->get_puyo(left_pos) != PuyoColor::EMPTY);
    bool right_blocked = !right_pos.is_valid() || 
                        (field_ && field_->get_puyo(right_pos) != PuyoColor::EMPTY);
    
    return left_blocked && right_blocked;
}

// クイックターン実行（180度回転）
bool PuyoController::perform_quick_turn() {
    PuyoPair quick_turned = current_pair_;
    
    // 180度回転（2回回転と同じ効果）
    switch (current_pair_.rot) {
        case Rotation::UP:    quick_turned.rot = Rotation::DOWN;  break;
        case Rotation::RIGHT: quick_turned.rot = Rotation::LEFT;  break;
        case Rotation::DOWN:  quick_turned.rot = Rotation::UP;    break;
        case Rotation::LEFT:  quick_turned.rot = Rotation::RIGHT; break;
    }
    
    if (is_valid_position(quick_turned)) {
        current_pair_ = quick_turned;
        return true;
    }
    
    return false;
}

bool PuyoController::place_current_pair() {
    if (!field_) {
        return false;
    }
    
    return field_->place_puyo_pair(current_pair_);
}

bool PuyoController::can_move_left() const {
    PuyoPair test_pair = current_pair_;
    test_pair.pos.x--;
    return is_valid_position(test_pair);
}

bool PuyoController::can_move_right() const {
    PuyoPair test_pair = current_pair_;
    test_pair.pos.x++;
    return is_valid_position(test_pair);
}

bool PuyoController::can_move_down() const {
    PuyoPair test_pair = current_pair_;
    test_pair.pos.y--;
    return is_valid_position(test_pair);
}

bool PuyoController::can_rotate_clockwise() const {
    PuyoPair rotated = create_rotated_pair(current_pair_, true);
    
    // 通常回転が可能か
    if (is_valid_position(rotated)) {
        return true;
    }
    
    // キック可能か
    std::vector<int> kick_offsets = {-1, 1};
    for (int offset : kick_offsets) {
        PuyoPair kicked = rotated;
        kicked.pos.x += offset;
        if (is_valid_position(kicked)) {
            return true;
        }
    }
    
    // クイックターン可能か
    return can_perform_quick_turn();
}

bool PuyoController::can_rotate_counter_clockwise() const {
    PuyoPair rotated = create_rotated_pair(current_pair_, false);
    
    // 通常回転が可能か
    if (is_valid_position(rotated)) {
        return true;
    }
    
    // キック可能か
    std::vector<int> kick_offsets = {-1, 1};
    for (int offset : kick_offsets) {
        PuyoPair kicked = rotated;
        kicked.pos.x += offset;
        if (is_valid_position(kicked)) {
            return true;
        }
    }
    
    // クイックターン可能か
    return can_perform_quick_turn();
}

PuyoPair PuyoController::create_rotated_pair(const PuyoPair& pair, bool clockwise) const {
    PuyoPair rotated = pair;
    
    if (clockwise) {
        // 時計回り
        switch (pair.rot) {
            case Rotation::UP:    rotated.rot = Rotation::RIGHT; break;
            case Rotation::RIGHT: rotated.rot = Rotation::DOWN;  break;
            case Rotation::DOWN:  rotated.rot = Rotation::LEFT;  break;
            case Rotation::LEFT:  rotated.rot = Rotation::UP;    break;
        }
    } else {
        // 反時計回り
        switch (pair.rot) {
            case Rotation::UP:    rotated.rot = Rotation::LEFT;  break;
            case Rotation::LEFT:  rotated.rot = Rotation::DOWN;  break;
            case Rotation::DOWN:  rotated.rot = Rotation::RIGHT; break;
            case Rotation::RIGHT: rotated.rot = Rotation::UP;    break;
        }
    }
    
    return rotated;
}

bool PuyoController::is_valid_position(const PuyoPair& pair) const {
    if (!field_) {
        return false;
    }
    
    return field_->can_place_puyo_pair(pair);
}

} // namespace puyo