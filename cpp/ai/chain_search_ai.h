#pragma once

#include "ai_base.h"
#include "core/field.h"
#include <vector>
#include <memory>
#include <climits>
#include <map>

namespace puyo {
namespace ai {

// 連鎖探索AI（簡易実装版）
class ChainSearchAI : public AIBase {
private:
    int search_depth_;
    double chain_weight_;
    double stability_weight_;
    
public:
    ChainSearchAI(const AIParameters& params = {}) 
        : AIBase("ChainSearchAI") {
        
        // パラメータの設定
        for (const auto& param : params) {
            set_parameter(param.first, param.second);
        }
        
        // デフォルト設定
        search_depth_ = std::stoi(get_parameter("search_depth", "3"));
        chain_weight_ = std::stod(get_parameter("chain_weight", "10.0"));
        stability_weight_ = std::stod(get_parameter("stability_weight", "1.0"));
        
        // 探索深度の制限
        if (search_depth_ < 1) search_depth_ = 1;
        if (search_depth_ > 4) search_depth_ = 4; // 簡易版では深度4まで
    }
    
    bool initialize() override {
        return AIBase::initialize();
    }
    
    AIDecision think(const GameState& state) override {
        if (!initialized_) {
            return AIDecision(-1, 0, {}, 0.0, "AI not initialized");
        }
        
        if (!state.own_field) {
            return AIDecision(-1, 0, {}, 0.0, "Field not available");
        }
        
        // 配置可能な(x, r)の組み合わせを列挙
        std::vector<std::pair<int, int>> valid_positions = get_valid_positions(*state.own_field);
        
        if (valid_positions.empty()) {
            return AIDecision(-1, 0, {}, 0.0, "No valid positions available");
        }
        
        // 各位置を評価して最良手を選択
        std::pair<int, int> best_move = {-1, -1};
        double best_score = -std::numeric_limits<double>::max();
        
        for (const auto& pos : valid_positions) {
            double score = evaluate_position(*state.own_field, pos.first, pos.second);
            
            if (score > best_score) {
                best_score = score;
                best_move = pos;
            }
        }
        
        if (best_move.first == -1) {
            // フォールバック：最初の有効な位置を選択
            best_move = valid_positions[0];
        }
        
        // MoveCommandリストを生成
        auto move_commands = MoveCommandGenerator::generate_move_commands(
            *state.own_field, best_move.first, best_move.second);
        
        // 確信度を計算
        double confidence = std::min(1.0, best_score / 100.0);
        if (confidence < 0.1) confidence = 0.1;
        
        std::string reason = "Chain search evaluation: score=" + 
                           std::to_string(best_score) + 
                           " at (" + std::to_string(best_move.first) + 
                           ", " + rotation_to_string(best_move.second) + ")";
        
        return AIDecision(best_move.first, best_move.second, move_commands, confidence, reason);
    }
    
    std::string get_type() const override {
        return "ChainSearch";
    }
    
    std::string get_debug_info() const override {
        return "ChainSearchAI depth=" + std::to_string(search_depth_) + 
               " chain_weight=" + std::to_string(chain_weight_);
    }
    
    int get_think_time_ms() const override {
        return search_depth_ * 100; // 深度1あたり100ms
    }

private:
    // 配置可能位置の取得
    std::vector<std::pair<int, int>> get_valid_positions(const Field& field) {
        std::vector<std::pair<int, int>> positions;
        
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            for (int r = 0; r < 4; ++r) {
                if (field.can_place(x, r)) {
                    positions.push_back({x, r});
                }
            }
        }
        
        return positions;
    }
    
    // 位置評価関数
    double evaluate_position(const Field& field, int x, int r) {
        double score = 0.0;
        
        // 1. 中央寄りの位置にボーナス
        int center_distance = std::abs(x - FIELD_WIDTH / 2);
        score += (3 - center_distance) * 2.0;
        
        // 2. フィールドの高さを考慮（低い方が良い）
        int height = get_column_height(field, x);
        score += (FIELD_HEIGHT - height) * 1.5;
        
        // 3. 同色ぷよの連結性を評価
        score += evaluate_connectivity(field, x) * chain_weight_;
        
        // 4. フィールドの安定性を評価
        score += evaluate_stability(field, x) * stability_weight_;
        
        // 5. ゲームオーバーを避ける
        if (height >= FIELD_HEIGHT - 2) {
            score -= 50.0; // 高すぎる列は大幅減点
        }
        
        return score;
    }
    
    // 列の高さを取得
    int get_column_height(const Field& field, int x) {
        for (int y = FIELD_HEIGHT - 1; y >= 0; --y) {
            if (field.get_puyo(Position(x, y)) != PuyoColor::EMPTY) {
                return FIELD_HEIGHT - y;
            }
        }
        return 0;
    }
    
    // 連結性評価
    double evaluate_connectivity(const Field& field, int x) {
        double connectivity = 0.0;
        
        // 周辺の同色ぷよをチェック
        std::map<PuyoColor, int> color_count;
        
        // 隣接セルをチェック
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};
        
        int y = get_column_height(field, x);
        if (y >= FIELD_HEIGHT) y = FIELD_HEIGHT - 1;
        
        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            if (nx >= 0 && nx < FIELD_WIDTH && ny >= 0 && ny < FIELD_HEIGHT) {
                PuyoColor color = field.get_puyo(Position(nx, ny));
                if (color != PuyoColor::EMPTY) {
                    color_count[color]++;
                }
            }
        }
        
        // 最も多い色に対してボーナス
        int max_count = 0;
        for (const auto& pair : color_count) {
            if (pair.second > max_count) {
                max_count = pair.second;
            }
        }
        
        connectivity = max_count * 5.0;
        
        return connectivity;
    }
    
    // 安定性評価
    double evaluate_stability(const Field& field, int x) {
        double stability = 0.0;
        
        // 列の均一性をチェック
        std::vector<int> heights;
        for (int col = 0; col < FIELD_WIDTH; ++col) {
            heights.push_back(get_column_height(field, col));
        }
        
        // 隣接列との高さの差が小さいほど安定
        if (x > 0) {
            int diff = std::abs(heights[x] - heights[x-1]);
            stability += std::max(0, 5 - diff);
        }
        if (x < FIELD_WIDTH - 1) {
            int diff = std::abs(heights[x] - heights[x+1]);
            stability += std::max(0, 5 - diff);
        }
        
        return stability;
    }
    
    // 回転状態を文字列に変換
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