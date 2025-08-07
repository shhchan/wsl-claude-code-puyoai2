#include "score_calculator.h"
#include <algorithm>

namespace puyo {

// 連鎖ボーナステーブル
const std::vector<int> ScoreCalculator::CHAIN_BONUS_TABLE = {
    0,   // 1連鎖
    8,   // 2連鎖
    16,  // 3連鎖
    32,  // 4連鎖
    64,  // 5連鎖
    96,  // 6連鎖
    128, // 7連鎖
    160, // 8連鎖
    192, // 9連鎖
    224, // 10連鎖
    256, // 11連鎖
    288, // 12連鎖
    320  // 13連鎖
    // 14連鎖以降は 320 + 32 * (N - 13)
};

// 連結ボーナステーブル
const std::vector<int> ScoreCalculator::CONNECTION_BONUS_TABLE = {
    0,  // 4個未満（使用されない）
    0,  // 4個
    2,  // 5個
    3,  // 6個
    4,  // 7個
    5,  // 8個
    6,  // 9個
    7,  // 10個
    10  // 11個以上
};

// 色数ボーナステーブル
const std::vector<int> ScoreCalculator::COLOR_BONUS_TABLE = {
    0,  // 1色
    3,  // 2色
    6,  // 3色
    12, // 4色
    24  // 5色
};

ScoreCalculator::ScoreCalculator() : pending_all_clear_bonus_(0) {}

ScoreResult ScoreCalculator::calculate_chain_score(const std::vector<ChainResult>& chain_results,
                                                  const Field& field_after_chain) {
    ScoreResult result;
    
    if (chain_results.empty()) {
        return result;
    }
    
    // 各連鎖のスコアを計算
    for (const auto& chain_result : chain_results) {
        int chain_score = calculate_single_chain_score(chain_result);
        result.chain_score += chain_score;
    }
    
    // 全消し判定
    result.is_all_clear = is_all_clear(field_after_chain);
    
    // 保留中の全消しボーナスを加算
    result.all_clear_bonus = pending_all_clear_bonus_;
    pending_all_clear_bonus_ = 0;
    
    // 全消しボーナスを次回に持ち越し
    if (result.is_all_clear) {
        pending_all_clear_bonus_ = 2100;
    }
    
    // 総得点計算
    result.total_score = result.chain_score + result.drop_score + result.all_clear_bonus;
    
    return result;
}

int ScoreCalculator::calculate_drop_bonus(int drop_height) {
    // 軸ぷよの落下距離1段につき1点 + 設置時1点
    return drop_height + 1;
}

bool ScoreCalculator::is_all_clear(const Field& field) const {
    // フィールドが完全に空かチェック
    for (int x = 0; x < FIELD_WIDTH; ++x) {
        for (int y = 0; y < FIELD_HEIGHT - 1; ++y) {  // 14段目は除く
            if (field.get_puyo(Position(x, y)) != PuyoColor::EMPTY) {
                return false;
            }
        }
    }
    return true;
}

int ScoreCalculator::calculate_single_chain_score(const ChainResult& chain_result) {
    if (chain_result.groups.empty()) {
        return 0;
    }
    
    // 基本得点計算式: 消したぷよの個数 × (連鎖ボーナス + 連結ボーナス + 色数ボーナス) × 10
    int chain_bonus = get_chain_bonus(chain_result.chain_level);
    int color_bonus = get_color_bonus(chain_result.color_count);
    
    // 連結ボーナスは各グループの最大連結数を使用
    int max_connection_bonus = 0;
    for (const auto& group : chain_result.groups) {
        int connection_bonus = get_connection_bonus(group.size());
        max_connection_bonus = std::max(max_connection_bonus, connection_bonus);
    }
    
    int total_bonus = chain_bonus + max_connection_bonus + color_bonus;
    
    // ボーナス合計が0の場合の特例処理
    if (total_bonus == 0 && chain_result.total_cleared == 4) {
        // 1連鎖4個消しの特例：40点
        return 40;
    }
    
    return chain_result.total_cleared * total_bonus * 10;
}

int ScoreCalculator::get_chain_bonus(int chain_level) const {
    if (chain_level <= 0) {
        return 0;
    }
    
    if (chain_level <= static_cast<int>(CHAIN_BONUS_TABLE.size())) {
        return CHAIN_BONUS_TABLE[chain_level - 1];
    }
    
    // 14連鎖以降の計算: 128 + 32 * (N - 7)
    return 128 + 32 * (chain_level - 7);
}

int ScoreCalculator::get_connection_bonus(int connection_count) const {
    if (connection_count == 4) return 0;
    if (connection_count == 5) return 2;
    if (connection_count == 6) return 3;
    if (connection_count == 7) return 4;
    if (connection_count == 8) return 5;
    if (connection_count == 9) return 6;
    if (connection_count == 10) return 7;
    if (connection_count >= 11) return 10;
    
    return 0;  // 4個未満
}

int ScoreCalculator::get_color_bonus(int color_count) const {
    if (color_count <= 0 || color_count > static_cast<int>(COLOR_BONUS_TABLE.size())) {
        return 0;
    }
    
    return COLOR_BONUS_TABLE[color_count - 1];
}

} // namespace puyo