#include "chain_detector.h"
#include <queue>
#include <algorithm>

namespace puyo {

ChainDetector::ChainDetector(Field* field) : field_(field) {}

ChainResult ChainDetector::detect_chain() {
    ChainResult result;
    
    if (!field_) {
        return result;
    }
    
    // 全ての連鎖グループを検出
    result.groups = find_all_chain_groups();
    
    if (result.groups.empty()) {
        return result;  // 連鎖なし
    }
    
    // 統計情報を計算
    result.total_cleared = 0;
    std::set<PuyoColor> colors_involved;
    
    for (const auto& group : result.groups) {
        result.total_cleared += group.size();
        colors_involved.insert(group.color);
    }
    
    result.color_count = static_cast<int>(colors_involved.size());
    
    return result;
}

std::vector<ChainResult> ChainDetector::execute_all_chains() {
    std::vector<ChainResult> all_chain_results;
    
    if (!field_) {
        return all_chain_results;
    }
    
    int chain_level = 1;
    
    while (true) {
        // 連鎖検出
        ChainResult result = detect_chain();
        
        if (!result.has_chains()) {
            break;  // もう連鎖がない
        }
        
        result.chain_level = chain_level;
        all_chain_results.push_back(result);
        
        // 連鎖グループを消去
        clear_chain_groups(result.groups);
        
        // 重力適用
        field_->apply_gravity();
        
        chain_level++;
    }
    
    return all_chain_results;
}

ChainGroup ChainDetector::find_connected_group(const Position& start_pos, 
                                              std::set<Position>& visited) const {
    ChainGroup group;
    
    if (!field_ || visited.count(start_pos)) {
        return group;
    }
    
    PuyoColor target_color = field_->get_puyo(start_pos);
    
    // 空またはおじゃまぷよは連鎖しない
    if (target_color == PuyoColor::EMPTY || target_color == PuyoColor::GARBAGE) {
        return group;
    }
    
    group.color = target_color;
    
    // BFS で連結グループを探索
    std::queue<Position> queue;
    std::set<Position> local_visited;
    
    queue.push(start_pos);
    local_visited.insert(start_pos);
    
    while (!queue.empty()) {
        Position current = queue.front();
        queue.pop();
        
        group.positions.push_back(current);
        visited.insert(current);
        
        // 隣接位置をチェック
        for (const Position& adj_pos : get_adjacent_positions(current)) {
            if (local_visited.count(adj_pos) || visited.count(adj_pos)) {
                continue;
            }
            
            PuyoColor adj_color = field_->get_puyo(adj_pos);
            if (adj_color == target_color) {
                queue.push(adj_pos);
                local_visited.insert(adj_pos);
            }
        }
    }
    
    return group;
}

std::vector<ChainGroup> ChainDetector::find_all_chain_groups() const {
    std::vector<ChainGroup> chain_groups;
    
    if (!field_) {
        return chain_groups;
    }
    
    std::set<Position> visited;
    
    // フィールド全体をスキャン
    for (int x = 0; x < FIELD_WIDTH; ++x) {
        for (int y = 0; y < FIELD_HEIGHT; ++y) {
            Position pos(x, y);
            
            if (visited.count(pos)) {
                continue;
            }
            
            ChainGroup group = find_connected_group(pos, visited);
            
            // 4個以上の連結グループのみ連鎖対象
            if (group.size() >= 4) {
                chain_groups.push_back(group);
            }
        }
    }
    
    return chain_groups;
}

void ChainDetector::clear_chain_groups(const std::vector<ChainGroup>& groups) {
    if (!field_) {
        return;
    }
    
    for (const auto& group : groups) {
        for (const auto& pos : group.positions) {
            field_->remove_puyo(pos);
        }
    }
}

bool ChainDetector::is_adjacent(const Position& pos1, const Position& pos2) const {
    int dx = std::abs(pos1.x - pos2.x);
    int dy = std::abs(pos1.y - pos2.y);
    
    // 上下左右に隣接（斜めは含まない）
    return (dx == 1 && dy == 0) || (dx == 0 && dy == 1);
}

std::vector<Position> ChainDetector::get_adjacent_positions(const Position& pos) const {
    std::vector<Position> adjacent;
    
    const std::array<std::pair<int, int>, 4> directions = {{
        {0, 1},   // 上
        {0, -1},  // 下
        {-1, 0},  // 左
        {1, 0}    // 右
    }};
    
    for (const auto& dir : directions) {
        Position adj_pos(pos.x + dir.first, pos.y + dir.second);
        
        if (adj_pos.is_valid()) {
            adjacent.push_back(adj_pos);
        }
    }
    
    return adjacent;
}

} // namespace puyo