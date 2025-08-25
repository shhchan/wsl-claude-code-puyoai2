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
    std::uniform_int_distribution<> dis_;
    
    // 実行可能なコマンドのリスト
    std::vector<MoveCommand> available_commands_;
    
public:
    RandomAI(const AIParameters& params = {}) 
        : AIBase("RandomAI"), gen_(rd_()), dis_(0, 4) {
        
        // 使用するコマンドを設定
        available_commands_ = {
            MoveCommand::LEFT,
            MoveCommand::RIGHT,
            MoveCommand::ROTATE_CW,
            MoveCommand::ROTATE_CCW,
            MoveCommand::DROP
        };
        
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
            return AIDecision(MoveCommand::NONE, 0.0, "AI not initialized");
        }
        
        // ランダムにコマンドを選択
        int index = dis_(gen_) % available_commands_.size();
        MoveCommand selected_command = available_commands_[index];
        
        // 選択したコマンドが実行可能かチェック（簡易版）
        // より詳細なチェックは実際のゲーム実行時に行う
        std::string reason = "Random selection: " + command_to_string(selected_command);
        
        return AIDecision(selected_command, 1.0, reason);
    }
    
    std::string get_type() const override {
        return "Random";
    }
    
    std::string get_debug_info() const override {
        return "RandomAI using seed: " + get_parameter("seed", "auto");
    }
    
private:
    std::string command_to_string(MoveCommand cmd) const {
        switch (cmd) {
            case MoveCommand::LEFT: return "LEFT";
            case MoveCommand::RIGHT: return "RIGHT";
            case MoveCommand::ROTATE_CW: return "ROTATE_CW";
            case MoveCommand::ROTATE_CCW: return "ROTATE_CCW";
            case MoveCommand::DROP: return "DROP";
            case MoveCommand::NONE: return "NONE";
            default: return "UNKNOWN";
        }
    }
};

} // namespace ai
} // namespace puyo