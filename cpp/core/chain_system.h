#pragma once

#include "chain_detector.h"
#include "score_calculator.h"
#include "field.h"

namespace puyo {

// 連鎖システム全体の結果
struct ChainSystemResult {
    std::vector<ChainResult> chain_results;  // 各連鎖の詳細結果
    ScoreResult score_result;                // スコア計算結果
    int total_chains;                        // 総連鎖数
    
    ChainSystemResult() : total_chains(0) {}
    
    bool has_chains() const { return total_chains > 0; }
};

class ChainSystem {
private:
    ChainDetector detector_;
    ScoreCalculator calculator_;
    Field* field_;
    
public:
    explicit ChainSystem(Field* field);
    
    // 連鎖の検出・実行・スコア計算を一括実行
    ChainSystemResult execute_chains();
    
    // 落下ボーナス付きで連鎖実行
    ChainSystemResult execute_chains_with_drop_bonus(int drop_height);
    
    // スコア計算機の取得（全消しボーナス管理用）
    ScoreCalculator& get_score_calculator() { return calculator_; }
    const ScoreCalculator& get_score_calculator() const { return calculator_; }
    
    // 統計情報の取得
    bool would_cause_chain() const;  // 現在の状態で連鎖が発生するか
    int count_potential_chains() const;  // 発生しうる連鎖数の推定
    
    // デバッグ用
    std::string get_chain_info(const ChainSystemResult& result) const;
};

} // namespace puyo