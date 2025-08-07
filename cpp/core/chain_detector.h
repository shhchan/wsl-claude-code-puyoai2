#pragma once

#include "puyo_types.h"
#include "field.h"
#include <vector>
#include <set>

namespace puyo {

// 連鎖結果情報
struct ChainGroup {
    PuyoColor color;
    std::vector<Position> positions;  // 消去されるぷよの位置
    int size() const { return static_cast<int>(positions.size()); }
};

struct ChainResult {
    std::vector<ChainGroup> groups;   // 同時に消える色グループ
    int chain_level;                  // 連鎖レベル（1連鎖、2連鎖...）
    int total_cleared;                // 消去ぷよ総数
    int color_count;                  // 消去に関わった色数
    
    bool has_chains() const { return !groups.empty(); }
    void clear() { groups.clear(); chain_level = 0; total_cleared = 0; color_count = 0; }
};

class ChainDetector {
private:
    Field* field_;
    
public:
    explicit ChainDetector(Field* field);
    
    // 連鎖検出
    ChainResult detect_chain();
    
    // 連鎖実行（検出 + 消去 + 落下を繰り返し）
    std::vector<ChainResult> execute_all_chains();
    
    // 指定位置から同色の連結グループを検出
    ChainGroup find_connected_group(const Position& start_pos, 
                                   std::set<Position>& visited) const;
    
    // フィールドから連鎖グループを検出
    std::vector<ChainGroup> find_all_chain_groups() const;
    
    // 連鎖グループをフィールドから消去
    void clear_chain_groups(const std::vector<ChainGroup>& groups);
    
    // おじゃまぷよの巻き込み消去
    void clear_adjacent_garbage(const std::vector<ChainGroup>& groups);
    
private:
    // 位置の隣接チェック
    bool is_adjacent(const Position& pos1, const Position& pos2) const;
    
    // 指定位置の隣接位置を取得
    std::vector<Position> get_adjacent_positions(const Position& pos) const;
};

} // namespace puyo