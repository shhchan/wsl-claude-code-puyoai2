#include "next_generator.h"
#include <algorithm>
#include <sstream>
#include <chrono>

namespace puyo {

NextGenerator::NextGenerator() {
    // 現在時刻をシードとして使用
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    unsigned int seed = static_cast<unsigned int>(duration.count());
    rng_.seed(seed);
    
    // デフォルト4色を設定（紫を除外）
    active_colors_ = {PuyoColor::RED, PuyoColor::GREEN, PuyoColor::BLUE, PuyoColor::YELLOW};
}

NextGenerator::NextGenerator(unsigned int seed) : rng_(seed) {
    // デフォルト4色を設定（紫を除外）
    active_colors_ = {PuyoColor::RED, PuyoColor::GREEN, PuyoColor::BLUE, PuyoColor::YELLOW};
}

void NextGenerator::set_active_colors(const std::vector<PuyoColor>& colors) {
    if (colors.size() != 4) {
        return;  // 必ず4色でなければならない
    }
    
    active_colors_ = colors;
    
    // 既存のNEXTを再生成
    initialize_next_sequence();
}

void NextGenerator::initialize_next_sequence() {
    for (int i = 0; i < 3; ++i) {
        next_pairs_[i] = generate_random_pair();
    }
}

PuyoPair NextGenerator::get_next_pair(int index) const {
    if (index < 0 || index >= 3) {
        return PuyoPair();  // 不正なインデックスの場合は空のペアを返す
    }
    
    return next_pairs_[index];
}

void NextGenerator::advance_to_next() {
    // 配列を左にシフト
    next_pairs_[0] = next_pairs_[1];
    next_pairs_[1] = next_pairs_[2];
    
    // 新しい「その次」を生成
    next_pairs_[2] = generate_random_pair();
}

std::string NextGenerator::to_string() const {
    std::ostringstream oss;
    
    const std::array<std::string, 3> labels = {"Current", "Next", "Next+1"};
    
    for (int i = 0; i < 3; ++i) {
        oss << labels[i] << ": ";
        
        // 軸ぷよ
        switch (next_pairs_[i].axis) {
            case PuyoColor::RED:    oss << "R"; break;
            case PuyoColor::GREEN:  oss << "G"; break;
            case PuyoColor::BLUE:   oss << "B"; break;
            case PuyoColor::YELLOW: oss << "Y"; break;
            case PuyoColor::PURPLE: oss << "P"; break;
            default:                oss << "?"; break;
        }
        
        oss << "-";
        
        // 子ぷよ
        switch (next_pairs_[i].child) {
            case PuyoColor::RED:    oss << "R"; break;
            case PuyoColor::GREEN:  oss << "G"; break;
            case PuyoColor::BLUE:   oss << "B"; break;
            case PuyoColor::YELLOW: oss << "Y"; break;
            case PuyoColor::PURPLE: oss << "P"; break;
            default:                oss << "?"; break;
        }
        
        if (i < 2) oss << ", ";
    }
    
    return oss.str();
}

PuyoPair NextGenerator::generate_random_pair() {
    PuyoColor axis = get_random_color();
    PuyoColor child = get_random_color();
    
    // デフォルト位置で生成
    return PuyoPair(axis, child);
}

PuyoColor NextGenerator::get_random_color() {
    if (active_colors_.empty()) {
        return PuyoColor::RED;  // フォールバック
    }
    
    std::uniform_int_distribution<size_t> dist(0, active_colors_.size() - 1);
    size_t index = dist(rng_);
    return active_colors_[index];
}

} // namespace puyo