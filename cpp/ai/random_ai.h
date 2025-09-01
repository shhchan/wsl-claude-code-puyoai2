#pragma once

#include "ai_base.h"
#include <random>
#include <vector>

namespace puyo {
namespace ai {

class RandomAI : public AIBase {
private:
    std::random_device rd_;
    std::mt19937 gen_;
    
public:
    RandomAI(const AIParameters& params = {}) 
        : AIBase("RandomAI"), gen_(rd_()) {
        
        // パラメータの設定
        for (const auto& param : params) {
            set_parameter(param.first, param.second);
        }
        
        // シード値の設定（パラメータで指定可能）
        std::string seed_str = get_parameter("seed");
        if (!seed_str.empty()) {
            try {
                unsigned int seed = static_cast<unsigned int>(std::stoul(seed_str));
                gen_.seed(seed);
            } catch (...) {
                // シード値が無効な場合はランダムシードを使用
            }
        }
    }
    
    bool initialize() override {
        if (!AIBase::initialize()) {
            return false;
        }
        
        // 追加の初期化処理があればここに記述
        return true;
    }
    
    AIDecision think(const GameState& state) override {
        if (!initialized_) {
            return AIDecision(-1, 0, {}, 0.0, "AI not initialized");
        }
        
        if (!state.own_field) {
            return AIDecision(-1, 0, {}, 0.0, "Field not available");
        }
        
        // 配置可能な(x, r)の組み合わせを列挙
        std::vector<std::pair<int, int>> valid_positions;
        
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            for (int r = 0; r < 4; ++r) { // 0:UP, 1:RIGHT, 2:DOWN, 3:LEFT
                if (state.own_field->can_place(x, r)) {
                    valid_positions.push_back({x, r});
                }
            }
        }
        
        if (valid_positions.empty()) {
            return AIDecision(-1, 0, {}, 0.0, "No valid positions available");
        }
        
        // ランダムに1つ選択
        std::uniform_int_distribution<> dis(0, valid_positions.size() - 1);
        int selected_index = dis(gen_);
        auto selected_position = valid_positions[selected_index];
        
        int target_x = selected_position.first;
        int target_r = selected_position.second;
        
        // MoveCommandリストを生成
        auto move_commands = MoveCommandGenerator::generate_move_commands(*state.own_field, target_x, target_r);
        
        std::string reason = "Random placement at (" + std::to_string(target_x) + ", " + 
                           rotation_to_string(target_r) + ")";
        
        return AIDecision(target_x, target_r, move_commands, 1.0, reason);
    }
    
    std::string get_type() const override {
        return "Random";
    }
    
    std::string get_debug_info() const override {
        return "RandomAI using seed: " + get_parameter("seed", "auto");
    }
    
private:
    std::string rotation_to_string(int r) const {
        switch (r) {
            case 0: return "UP";
            case 1: return "RIGHT";
            case 2: return "DOWN";
            case 3: return "LEFT";
            default: return "UNKNOWN";
        }
    }
};

} // namespace ai
} // namespace puyo