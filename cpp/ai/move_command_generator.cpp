#include "ai_base.h"
#include <iostream>

namespace puyo {
namespace ai {

std::vector<MoveCommand> MoveCommandGenerator::generate_move_commands(const Field& field, int target_x, int target_r) {
    // まず基本アルゴリズムを試行
    bool has_12_height_column = false;
    for (int col = 0; col < FIELD_WIDTH; ++col) {
        int height = 0;
        for (int row = 0; row < FIELD_HEIGHT; ++row) {
            if (field.get_puyo(Position(col, row)) != PuyoColor::EMPTY) {
                height = row + 1;
            } else {
                break;
            }
        }
        if (height >= 12) {
            has_12_height_column = true;
            break;
        }
    }
    
    if (!has_12_height_column) {
        return generate_basic_commands(target_x, target_r);
    } else {
        return generate_advanced_commands(field, target_x, target_r);
    }
}

std::vector<MoveCommand> MoveCommandGenerator::generate_basic_commands(int target_x, int target_r) {
    std::vector<MoveCommand> commands;
    
    // 2列目（インデックス1）からスタート
    int current_x = 2;  // 0ベースで2列目
    int current_r = 0;  // UP状態からスタート
    
    // 水平移動
    if (target_x < current_x) {
        // 左移動
        for (int i = current_x; i > target_x; --i) {
            commands.push_back(MoveCommand::LEFT);
        }
    } else if (target_x > current_x) {
        // 右移動
        for (int i = current_x; i < target_x; ++i) {
            commands.push_back(MoveCommand::RIGHT);
        }
    }
    
    // 回転処理
    auto rotation_commands = generate_rotation_commands(current_r, target_r);
    commands.insert(commands.end(), rotation_commands.begin(), rotation_commands.end());
    
    // 最終的に落下操作を追加
    commands.push_back(MoveCommand::DROP);
    
    return commands;
}

std::vector<MoveCommand> MoveCommandGenerator::generate_advanced_commands(const Field& field, int target_x, int target_r) {
    std::vector<MoveCommand> commands;
    
    // 到達可能な列を算出
    std::set<int> reachable = calculate_reachable_columns(field);

    // target_x が設置不可能な場合は例外を投げる
    if (!field.can_place(target_x, target_r)) {
        throw std::runtime_error("Cannot place at (" + std::to_string(target_x) + ", " + std::to_string(target_r) + ").");
    }
    
    // 11段列を検出
    std::vector<int> height_11_columns = find_11_height_columns(field, reachable);
    
    int current_x = 2;  // 2列目からスタート
    int current_r = 0;  // UP状態からスタート
    
    if (!height_11_columns.empty()) {
        // 11段列を利用した迂回ルート
        
        // 2列目に最も近い11段列を選択
        int detour_col = height_11_columns[0];
        int min_distance = abs(detour_col - 2);
        for (int col : height_11_columns) {
            int distance = abs(col - 2);
            if (distance < min_distance) {
                min_distance = distance;
                detour_col = col;
            }
        }
        
        // 迂回列へ移動
        if (detour_col < current_x) {
            // 左移動で到達
            for (int i = current_x; i > detour_col; --i) {
                commands.push_back(MoveCommand::LEFT);
            }
            // 右回転2回 (UP → DOWN)
            commands.push_back(MoveCommand::ROTATE_CW);
            commands.push_back(MoveCommand::ROTATE_CW);
            current_r = 2; // DOWN
            
            // 方向調整
            if (target_x < detour_col) {
                // 左回転1回 (DOWN → RIGHT)
                commands.push_back(MoveCommand::ROTATE_CCW);
                current_r = 1; // RIGHT
            } else if (target_x > detour_col) {
                // 右回転1回 (DOWN → LEFT)
                commands.push_back(MoveCommand::ROTATE_CW);
                current_r = 3; // LEFT
            }
        } else if (detour_col > current_x) {
            // 右移動で到達
            for (int i = current_x; i < detour_col; ++i) {
                commands.push_back(MoveCommand::RIGHT);
            }
            // 左回転2回 (UP → DOWN)
            commands.push_back(MoveCommand::ROTATE_CCW);
            commands.push_back(MoveCommand::ROTATE_CCW);
            current_r = 2; // DOWN

            // 方向調整
            if (target_x < detour_col) {
                // 左回転1回 (DOWN → RIGHT)
                commands.push_back(MoveCommand::ROTATE_CCW);
                current_r = 1; // RIGHT
            } else if (target_x > detour_col) {
                // 右回転1回 (DOWN → LEFT)
                commands.push_back(MoveCommand::ROTATE_CW);
                current_r = 3; // LEFT
            }
        }
        
        current_x = detour_col;
    }
    
    // 最終的な位置移動
    if (target_x < current_x) {
        for (int i = current_x; i > target_x; --i) {
            commands.push_back(MoveCommand::LEFT);
        }
    } else if (target_x > current_x) {
        for (int i = current_x; i < target_x; ++i) {
            commands.push_back(MoveCommand::RIGHT);
        }
    }
    
    // 最終的な回転調整
    auto final_rotation = generate_rotation_commands(current_r, target_r);
    commands.insert(commands.end(), final_rotation.begin(), final_rotation.end());
    
    // 落下操作
    commands.push_back(MoveCommand::DROP);
    
    return commands;
}

std::set<int> MoveCommandGenerator::calculate_reachable_columns(const Field& field) {
    std::set<int> reachable;
    
    // 2列目から左へ探索
    bool blocked = false;
    for (int col = 2; col >= 0 && !blocked; --col) {
        int height = 0;
        for (int row = 0; row < FIELD_HEIGHT; ++row) {
            if (field.get_puyo(Position(col, row)) != PuyoColor::EMPTY) {
                height = row + 1;
            } else {
                break;
            }
        }
        
        if (height < 12) {
            reachable.insert(col);
        } else {
            blocked = true;
        }
    }
    
    // 2列目から右へ探索
    blocked = false;
    for (int col = 2; col < FIELD_WIDTH && !blocked; ++col) {
        int height = 0;
        for (int row = 0; row < FIELD_HEIGHT; ++row) {
            if (field.get_puyo(Position(col, row)) != PuyoColor::EMPTY) {
                height = row + 1;
            } else {
                break;
            }
        }
        
        if (height < 12) {
            reachable.insert(col);
        } else {
            blocked = true;
        }
    }
    
    return reachable;
}

std::vector<int> MoveCommandGenerator::find_11_height_columns(const Field& field, const std::set<int>& reachable) {
    std::vector<int> height_11_columns;
    
    for (int col : reachable) {
        int height = 0;
        for (int row = 0; row < FIELD_HEIGHT; ++row) {
            if (field.get_puyo(Position(col, row)) != PuyoColor::EMPTY) {
                height = row + 1;
            } else {
                break;
            }
        }
        
        if (height == 11) {
            height_11_columns.push_back(col);
        }
    }
    
    return height_11_columns;
}

std::vector<MoveCommand> MoveCommandGenerator::generate_rotation_commands(int current_rotation, int target_rotation) {
    std::vector<MoveCommand> commands;
    
    if (current_rotation == target_rotation) {
        return commands;
    }
    
    // 回転差分を計算
    int diff = target_rotation - current_rotation;
    
    // 正規化 (-3 <= diff <= 3)
    while (diff > 2) diff -= 4;
    while (diff < -2) diff += 4;
    
    if (diff == 1) {
        // 右回転1回
        commands.push_back(MoveCommand::ROTATE_CW);
    } else if (diff == 2) {
        // 右回転2回
        commands.push_back(MoveCommand::ROTATE_CW);
        commands.push_back(MoveCommand::ROTATE_CW);
    } else if (diff == -1) {
        // 左回転1回
        commands.push_back(MoveCommand::ROTATE_CCW);
    } else if (diff == -2) {
        // 左回転2回
        commands.push_back(MoveCommand::ROTATE_CCW);
        commands.push_back(MoveCommand::ROTATE_CCW);
    }
    
    return commands;
}

} // namespace ai
} // namespace puyo