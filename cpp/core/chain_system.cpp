#include "chain_system.h"
#include <sstream>

namespace puyo {

ChainSystem::ChainSystem(Field* field) 
    : detector_(field), calculator_(), field_(field) {}

ChainSystemResult ChainSystem::execute_chains() {
    ChainSystemResult result;
    
    if (!field_) {
        return result;
    }
    
    // 連鎖の検出と実行
    result.chain_results = detector_.execute_all_chains();
    result.total_chains = static_cast<int>(result.chain_results.size());
    
    // スコア計算
    result.score_result = calculator_.calculate_chain_score(result.chain_results, *field_);
    
    return result;
}

ChainSystemResult ChainSystem::execute_chains_with_drop_bonus(int drop_height) {
    ChainSystemResult result = execute_chains();
    
    // 落下ボーナスを追加
    result.score_result.drop_score = calculator_.calculate_drop_bonus(drop_height);
    result.score_result.total_score += result.score_result.drop_score;
    
    return result;
}

bool ChainSystem::would_cause_chain() const {
    if (!field_) {
        return false;
    }
    
    // フィールドをコピーして連鎖検出
    Field temp_field = *field_;
    ChainDetector temp_detector(&temp_field);
    
    ChainResult test_result = temp_detector.detect_chain();
    return test_result.has_chains();
}

int ChainSystem::count_potential_chains() const {
    if (!field_) {
        return 0;
    }
    
    // フィールドをコピーして全連鎖実行
    Field temp_field = *field_;
    ChainDetector temp_detector(&temp_field);
    
    auto chain_results = temp_detector.execute_all_chains();
    return static_cast<int>(chain_results.size());
}

std::string ChainSystem::get_chain_info(const ChainSystemResult& result) const {
    std::ostringstream oss;
    
    if (!result.has_chains()) {
        oss << "No chains detected.";
        return oss.str();
    }
    
    oss << "Chain Summary:\n";
    oss << "Total Chains: " << result.total_chains << "\n";
    oss << "Total Score: " << result.score_result.total_score << "\n";
    
    if (result.score_result.chain_score > 0) {
        oss << "Chain Score: " << result.score_result.chain_score << "\n";
    }
    
    if (result.score_result.drop_score > 0) {
        oss << "Drop Bonus: " << result.score_result.drop_score << "\n";
    }
    
    if (result.score_result.all_clear_bonus > 0) {
        oss << "All Clear Bonus: " << result.score_result.all_clear_bonus << "\n";
    }
    
    if (result.score_result.is_all_clear) {
        oss << "ALL CLEAR! (Next chain gets +2100 bonus)\n";
    }
    
    oss << "\nChain Details:\n";
    for (size_t i = 0; i < result.chain_results.size(); ++i) {
        const auto& chain = result.chain_results[i];
        oss << "Chain " << (i + 1) << ": ";
        oss << chain.total_cleared << " puyos cleared, ";
        oss << chain.color_count << " colors involved\n";
        
        for (const auto& group : chain.groups) {
            oss << "  - ";
            switch (group.color) {
                case PuyoColor::RED:    oss << "Red"; break;
                case PuyoColor::GREEN:  oss << "Green"; break;
                case PuyoColor::BLUE:   oss << "Blue"; break;
                case PuyoColor::YELLOW: oss << "Yellow"; break;
                case PuyoColor::PURPLE: oss << "Purple"; break;
                default:                oss << "Unknown"; break;
            }
            oss << " x" << group.size() << "\n";
        }
    }
    
    return oss.str();
}

} // namespace puyo