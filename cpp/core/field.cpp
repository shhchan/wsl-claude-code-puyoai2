#include "field.h"
#include <sstream>

namespace puyo {

Field::Field() {
    clear();
}

void Field::clear() {
    field_bits_.clear_all();
    for (auto& used : row14_used_) {
        used = false;
    }
}

PuyoColor Field::get_puyo(const Position& pos) const {
    return field_bits_.get_color(pos);
}

void Field::set_puyo(const Position& pos, PuyoColor color) {
    field_bits_.set_color(pos, color);
}

void Field::remove_puyo(const Position& pos) {
    field_bits_.clear_position(pos);
}

bool Field::can_place_at_row14(int column) const {
    return column >= 0 && column < FIELD_WIDTH && !row14_used_[column];
}

void Field::mark_row14_used(int column) {
    if (column >= 0 && column < FIELD_WIDTH) {
        row14_used_[column] = true;
    }
}

bool Field::is_row14_used(int column) const {
    return column >= 0 && column < FIELD_WIDTH && row14_used_[column];
}

bool Field::can_place(int x, int r) const {
    // サンプルコードと同等の厳密なアルゴリズム
    // ぷよの高さ情報を取得
    uint8_t heights[6] = {};
    for (int i = 0; i < 6; ++i) {
        heights[i] = 0;
        for (int y = 0; y < FIELD_HEIGHT; ++y) {
            if (get_puyo(Position(i, y)) != PuyoColor::EMPTY) {
                heights[i] = y + 1;  // y座標+1が実際の高さ
            } else {
                break;
            }
        }
    }
    // 14段目の情報（bit列）
    uint8_t row14 = 0;
    for (int i = 0; i < 6; ++i) {
        if (is_row14_used(i)) row14 |= (1 << i);
    }
    // 回転方向のオフセット
    static const int dx[4] = {0, 1, 0, -1}; // UP, RIGHT, DOWN, LEFT
    static const int dy[4] = {1, 0, -1, 0};
    // 0:UP, 1:RIGHT, 2:DOWN, 3:LEFT
    int dir = r;
    // 軸ぷよが14段目
    if (heights[x] + (dir == 2) > 12) return false;
    int child_x = x + dx[dir];
    if (child_x < 0 || child_x >= 6) return false;
    int child_y = heights[child_x] + (dir == 0);
    if (child_y == 13 && ((row14 >> child_x) & 1)) return false;
    // チェックリスト
    static const int check[6][4] = {
        {1, 0, -1, -1}, {1, -1, -1, -1}, {-1, -1, -1, -1}, {3, -1, -1, -1}, {3, 4, -1, -1}, {3, 4, 5, -1}
    };
    static const int check_12[6][6] = {
        {1, 2, 3, 4, 5, -1}, {2, 3, 4, 5, -1, -1}, {-1, -1, -1, -1, -1, -1}, {2, 1, 0, -1, -1, -1}, {3, 2, 1, 0, -1, -1}, {4, 3, 2, 1, 0, -1}
    };
    int check_x = x;
    if (dir == 1 && x >= 2) check_x += 1;
    else if (dir == 3 && x <= 2) check_x -= 1;
    int height_12_idx = -1;
    for (int i = 0; check[check_x][i] != -1; ++i) {
        if (heights[check[check_x][i]] > 12) return false;
        if (heights[check[check_x][i]] == 12 && height_12_idx == -1) height_12_idx = check[check_x][i];
    }
    if (height_12_idx == -1) return true;
    if (heights[1] > 11 && heights[3] > 11) return true;
    for (int i = 0; check_12[height_12_idx][i] != -1; ++i) {
        if (heights[check_12[height_12_idx][i]] > 11) break;
        if (heights[check_12[height_12_idx][i]] == 11) return true;
    }
    return false;
}

bool Field::can_place_puyo_pair(const PuyoPair& pair) const {
    Position axis_pos = pair.pos;
    Position child_pos = pair.get_child_position();
    
    // 位置の有効性チェック
    if (!axis_pos.is_valid() || !child_pos.is_valid()) {
        return false;
    }
    
    // 軸ぷよの配置チェック
    if (get_puyo(axis_pos) != PuyoColor::EMPTY) {
        return false;
    }
    
    // 子ぷよの配置チェック
    if (get_puyo(child_pos) != PuyoColor::EMPTY) {
        return false;
    }
    
    // 14段目特殊仕様チェック
    if (axis_pos.y == FIELD_HEIGHT - 1) {  // 14段目
        if (!can_place_at_row14(axis_pos.x)) {
            return false;
        }
    }
    
    if (child_pos.y == FIELD_HEIGHT - 1) {  // 14段目
        if (!can_place_at_row14(child_pos.x)) {
            return false;
        }
    }
    
    return true;
}

bool Field::place_puyo_pair(const PuyoPair& pair) {
    if (!can_place_puyo_pair(pair)) {
        return false;
    }
    
    Position axis_pos = pair.pos;
    Position child_pos = pair.get_child_position();
    
    // ぷよを配置
    set_puyo(axis_pos, pair.axis);
    set_puyo(child_pos, pair.child);
    
    // 14段目使用フラグを設定
    if (axis_pos.y == FIELD_HEIGHT - 1) {
        mark_row14_used(axis_pos.x);
    }
    if (child_pos.y == FIELD_HEIGHT - 1) {
        mark_row14_used(child_pos.x);
    }
    
    return true;
}

// NOTE: この実装は分かりやすさを重視した基本実装
// ビット演算による高速化はチケット011で実装予定
bool Field::apply_gravity() {
    bool any_moved = false;
    
    // 各列について下から上へ処理
    for (int x = 0; x < FIELD_WIDTH; ++x) {
        std::vector<PuyoColor> column_puyos;
        
        // 14段目は落下対象外なので、まず14段目をチェック
        PuyoColor puyo_14 = get_puyo(Position(x, 13));
        
        // 1段目から13段目までのぷよを収集（空でないもののみ）
        for (int y = 0; y < FIELD_HEIGHT - 1; ++y) {
            PuyoColor puyo = get_puyo(Position(x, y));
            if (puyo != PuyoColor::EMPTY) {
                column_puyos.push_back(puyo);
            }
        }
        
        // 列をクリア（14段目は除く）
        for (int y = 0; y < FIELD_HEIGHT - 1; ++y) {
            remove_puyo(Position(x, y));
        }
        
        // 下から詰めて配置
        for (size_t i = 0; i < column_puyos.size(); ++i) {
            Position new_pos(x, static_cast<int>(i));
            set_puyo(new_pos, column_puyos[i]);
        }
        
        // 移動があったかチェック
        if (column_puyos.size() > 0) {
            // 元の位置と新しい位置を比較して移動判定
            // 簡単な判定：空きがあれば移動ありとする
            for (int y = static_cast<int>(column_puyos.size()); y < FIELD_HEIGHT - 1; ++y) {
                if (get_puyo(Position(x, y)) == PuyoColor::EMPTY) {
                    any_moved = true;
                    break;
                }
            }
        }
        
        // 14段目は元の位置に戻す
        if (puyo_14 != PuyoColor::EMPTY) {
            set_puyo(Position(x, 13), puyo_14);
        }
    }
    
    return any_moved;
}

bool Field::is_game_over() const {
    // 窒息点（3列目12段目）にぷよがあるかチェック
    Position choke_point(2, 11);  // 3列目12段目（0-indexed）
    return get_puyo(choke_point) != PuyoColor::EMPTY;
}

std::string Field::to_string() const {
    std::ostringstream oss;
    
    // 上から下へ表示（14段目から1段目）
    for (int y = FIELD_HEIGHT - 1; y >= 0; --y) {
        oss << "Row " << (y + 1) << ": ";
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            PuyoColor puyo = get_puyo(Position(x, y));
            
            switch (puyo) {
                case PuyoColor::EMPTY:   oss << "."; break;
                case PuyoColor::RED:     oss << "R"; break;
                case PuyoColor::GREEN:   oss << "G"; break;
                case PuyoColor::BLUE:    oss << "B"; break;
                case PuyoColor::YELLOW:  oss << "Y"; break;
                case PuyoColor::PURPLE:  oss << "P"; break;
                case PuyoColor::GARBAGE: oss << "X"; break;
                default:                 oss << "?"; break;
            }
        }
        
        // 14段目使用状況を表示
        if (y == FIELD_HEIGHT - 1) {
            oss << " (14th used: ";
            for (int x = 0; x < FIELD_WIDTH; ++x) {
                oss << (is_row14_used(x) ? "1" : "0");
            }
            oss << ")";
        }
        
        oss << "\n";
    }
    
    return oss.str();
}

} // namespace puyo