#include "garbage_system.h"
#include <algorithm>
#include <random>
#include <sstream>

namespace puyo {

GarbageSystem::GarbageSystem(Field* field) 
    : field_(field), total_pending_(0), accumulated_score_(0) {}

int GarbageSystem::calculate_garbage_to_send(int score) {
    if (score <= 0) return 0;
    
    // 蓄積スコアと今回のスコアを合計
    int total_score = accumulated_score_ + score;
    
    // おじゃまぷよ個数と残りスコアを計算
    int garbage_count = total_score / GARBAGE_RATE;
    accumulated_score_ = total_score % GARBAGE_RATE;
    
    return garbage_count;
}

void GarbageSystem::add_pending_garbage(int count, int source_player) {
    if (count <= 0) return;
    
    pending_garbage_.push(GarbagePuyo(count, source_player));
    total_pending_ += count;
}

int GarbageSystem::offset_garbage_with_score(int score) {
    if (score <= 0 || total_pending_ <= 0) return 0;
    
    // 得点から相殺可能なおじゃまぷよ数を計算（蓄積スコアは使わない）
    int offset_count = score / GARBAGE_RATE;
    int actual_offset = std::min(offset_count, total_pending_);
    
    // 予告おじゃまぷよから相殺分を減算
    int remaining_offset = actual_offset;
    
    while (remaining_offset > 0 && !pending_garbage_.empty()) {
        GarbagePuyo& front_garbage = pending_garbage_.front();
        
        if (front_garbage.count <= remaining_offset) {
            // このおじゃまぷよを完全に相殺
            remaining_offset -= front_garbage.count;
            pending_garbage_.pop();
        } else {
            // このおじゃまぷよを部分的に相殺
            front_garbage.count -= remaining_offset;
            remaining_offset = 0;
        }
    }
    
    total_pending_ -= actual_offset;
    
    return actual_offset;
}

GarbageResult GarbageSystem::drop_pending_garbage() {
    GarbageResult result;
    
    if (!field_ || total_pending_ <= 0) {
        return result;
    }
    
    result.received_garbage = total_pending_;
    
    // おじゃまぷよをフィールドに配置
    bool placement_success = place_garbage_on_field(total_pending_);
    
    if (placement_success) {
        result.placed_garbage = total_pending_;
        
        // 予告おじゃまぷよをクリア
        clear_pending_garbage();
    } else {
        // 配置失敗時の処理（フィールドが満杯など）
        result.placed_garbage = 0;
    }
    
    return result;
}

void GarbageSystem::clear_pending_garbage() {
    while (!pending_garbage_.empty()) {
        pending_garbage_.pop();
    }
    total_pending_ = 0;
}

bool GarbageSystem::place_garbage_on_field(int count) {
    if (!field_ || count <= 0) {
        return false;
    }
    
    // 配置位置を計算（N段+r個方式）
    std::vector<Position> positions = calculate_garbage_positions(count);
    
    if (positions.empty()) {
        return false;  // 配置不可
    }
    
    // おじゃまぷよを配置
    place_garbage_from_top(positions);
    
    return true;
}

std::string GarbageSystem::get_garbage_info() const {
    std::ostringstream oss;
    
    oss << "Pending Garbage: " << total_pending_ << " pieces\n";
    oss << "Accumulated Score: " << accumulated_score_ << " points\n";
    
    if (total_pending_ > 0) {
        oss << "Queue details:\n";
        std::queue<GarbagePuyo> temp_queue = pending_garbage_;
        int index = 1;
        
        while (!temp_queue.empty()) {
            const GarbagePuyo& garbage = temp_queue.front();
            oss << "  " << index << ": " << garbage.count << " pieces";
            if (garbage.source_player >= 0) {
                oss << " (from player " << garbage.source_player << ")";
            }
            oss << "\n";
            temp_queue.pop();
            index++;
        }
    }
    
    return oss.str();
}

std::vector<Position> GarbageSystem::calculate_garbage_positions(int count) {
    std::vector<Position> positions;
    
    if (!field_ || count <= 0) return positions;
    
    // N段+r個の計算
    int full_layers, remainder_count;
    calculate_layers_and_remainder(count, full_layers, remainder_count);
    
    // フィールドの現在の高さを調べる
    int max_height = 0;
    for (int x = 0; x < FIELD_WIDTH; ++x) {
        for (int y = FIELD_HEIGHT - 1; y >= 0; --y) {
            if (field_->get_puyo(Position(x, y)) != PuyoColor::EMPTY) {
                max_height = std::max(max_height, y + 1);
                break;
            }
        }
    }
    
    // 配置開始位置
    int start_y = max_height;
    
    // フル段の配置
    for (int layer = 0; layer < full_layers; ++layer) {
        int current_y = start_y + layer;
        if (current_y >= FIELD_HEIGHT - 1) break;  // 14段目は使用不可
        
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            positions.push_back(Position(x, current_y));
        }
    }
    
    // 余り個数の配置（ランダムな列に配置）
    if (remainder_count > 0) {
        int remainder_y = start_y + full_layers;
        if (remainder_y < FIELD_HEIGHT - 1) {  // 14段目は使用不可
            std::vector<int> selected_columns = select_random_columns(remainder_count);
            
            for (int col : selected_columns) {
                positions.push_back(Position(col, remainder_y));
            }
        }
    }
    
    return positions;
}

void GarbageSystem::calculate_layers_and_remainder(int count, int& full_layers, int& remainder_count) {
    full_layers = count / FIELD_WIDTH;
    remainder_count = count % FIELD_WIDTH;
}

std::vector<int> GarbageSystem::select_random_columns(int count) {
    std::vector<int> all_columns;
    for (int x = 0; x < FIELD_WIDTH; ++x) {
        all_columns.push_back(x);
    }
    
    // ランダムシャッフル
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(all_columns.begin(), all_columns.end(), g);
    
    // 必要な数だけ選択
    std::vector<int> selected_columns(all_columns.begin(), 
                                     all_columns.begin() + std::min(count, FIELD_WIDTH));
    
    // ソートして左から順に配置
    std::sort(selected_columns.begin(), selected_columns.end());
    
    return selected_columns;
}

void GarbageSystem::place_garbage_from_top(const std::vector<Position>& positions) {
    if (!field_) return;
    
    for (const Position& pos : positions) {
        field_->set_puyo(pos, PuyoColor::GARBAGE);
    }
}

} // namespace puyo