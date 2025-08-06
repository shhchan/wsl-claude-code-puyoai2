#pragma once

#include "puyo_types.h"
#include <random>
#include <array>
#include <vector>

namespace puyo {

class NextGenerator {
private:
    std::mt19937 rng_;
    std::vector<PuyoColor> active_colors_;  // ゲームで使用する色（4色）
    std::array<PuyoPair, 3> next_pairs_;    // NEXT[0]=現在, NEXT[1]=次, NEXT[2]=その次
    
public:
    NextGenerator();
    explicit NextGenerator(unsigned int seed);
    
    // 色設定
    void set_active_colors(const std::vector<PuyoColor>& colors);
    const std::vector<PuyoColor>& get_active_colors() const { return active_colors_; }
    
    // NEXT管理
    void initialize_next_sequence();
    PuyoPair get_current_pair() const { return next_pairs_[0]; }
    PuyoPair get_next_pair(int index) const;  // 0=現在, 1=次, 2=その次
    
    // 次のペアを生成して順送り
    void advance_to_next();
    
    // デバッグ用
    std::string to_string() const;
    
private:
    // ランダムぷよペア生成
    PuyoPair generate_random_pair();
    PuyoColor get_random_color();
};

} // namespace puyo