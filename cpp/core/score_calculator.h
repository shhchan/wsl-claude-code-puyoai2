#pragma once

#include "chain_detector.h"
#include <vector>

namespace puyo {

// スコア計算結果
struct ScoreResult {
    int chain_score;        // 連鎖による得点
    int drop_score;         // 落下ボーナス
    int all_clear_bonus;    // 全消しボーナス
    int total_score;        // 総得点
    bool is_all_clear;      // 全消しフラグ
    
    ScoreResult() : chain_score(0), drop_score(0), all_clear_bonus(0), 
                   total_score(0), is_all_clear(false) {}
};

class ScoreCalculator {
private:
    static const std::vector<int> CHAIN_BONUS_TABLE;
    static const std::vector<int> CONNECTION_BONUS_TABLE;
    static const std::vector<int> COLOR_BONUS_TABLE;
    
    int pending_all_clear_bonus_;  // 次回連鎖で加算される全消しボーナス
    
public:
    ScoreCalculator();
    
    // 連鎖結果からスコアを計算
    ScoreResult calculate_chain_score(const std::vector<ChainResult>& chain_results,
                                     const Field& field_after_chain);
    
    // 落下ボーナスの計算
    int calculate_drop_bonus(int drop_height);
    
    // 全消し判定
    bool is_all_clear(const Field& field) const;
    
    // 保留中の全消しボーナスを設定
    void set_pending_all_clear_bonus(int bonus) { pending_all_clear_bonus_ = bonus; }
    int get_pending_all_clear_bonus() const { return pending_all_clear_bonus_; }
    
    // リセット
    void reset() { pending_all_clear_bonus_ = 0; }
    
private:
    // 単一連鎖のスコア計算
    int calculate_single_chain_score(const ChainResult& chain_result);
    
    // 各種ボーナスの取得
    int get_chain_bonus(int chain_level) const;
    int get_connection_bonus(int connection_count) const;
    int get_color_bonus(int color_count) const;
};

} // namespace puyo