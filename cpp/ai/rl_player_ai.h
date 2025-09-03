#pragma once

#include "ai_base.h"
#include "core/field.h"
#include <vector>
#include <memory>
#include <random>
#include <map>
#include <fstream>

namespace puyo {
namespace ai {

// 強化学習用の状態表現
struct RLState {
    std::vector<int> field_state;  // フィールド状態のエンコーディング
    int current_colors[2];         // 現在のぷよペア色
    
    RLState() : field_state(FIELD_WIDTH * FIELD_HEIGHT, 0) {
        current_colors[0] = 0;
        current_colors[1] = 0;
    }
};

// Q値テーブルのエントリ
struct QEntry {
    double q_value;
    int visit_count;
    
    QEntry() : q_value(0.0), visit_count(0) {}
    QEntry(double q, int count) : q_value(q), visit_count(count) {}
};

// 経験データ
struct Experience {
    RLState state;
    std::pair<int, int> action;  // (x, r)
    double reward;
    RLState next_state;
    bool is_terminal;
    
    Experience(const RLState& s, const std::pair<int, int>& a, double r, 
              const RLState& next_s, bool terminal)
        : state(s), action(a), reward(r), next_state(next_s), is_terminal(terminal) {}
};

// 強化学習AI（Q-Learning実装）
class RLPlayerAI : public AIBase {
private:
    // 学習パラメータ
    double learning_rate_;
    double discount_factor_;
    double epsilon_;          // ε-greedy戦略のε
    double epsilon_decay_;
    double min_epsilon_;
    
    // Q値テーブル
    std::map<std::string, std::map<std::pair<int, int>, QEntry>> q_table_;
    
    // 経験リプレイ
    std::vector<Experience> experience_buffer_;
    int max_buffer_size_;
    
    // ランダム生成器
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_real_distribution<> uniform_dist_;
    
    // 学習統計
    int total_games_;
    double total_reward_;
    std::vector<double> reward_history_;
    
    // モデル保存関連
    std::string model_file_path_;
    
public:
    RLPlayerAI(const AIParameters& params = {}) 
        : AIBase("RLPlayerAI"), gen_(rd_()), uniform_dist_(0.0, 1.0) {
        
        // パラメータの設定
        for (const auto& param : params) {
            set_parameter(param.first, param.second);
        }
        
        // デフォルト学習パラメータ
        learning_rate_ = std::stod(get_parameter("learning_rate", "0.1"));
        discount_factor_ = std::stod(get_parameter("discount_factor", "0.9"));
        epsilon_ = std::stod(get_parameter("epsilon", "0.3"));
        epsilon_decay_ = std::stod(get_parameter("epsilon_decay", "0.995"));
        min_epsilon_ = std::stod(get_parameter("min_epsilon", "0.01"));
        max_buffer_size_ = std::stoi(get_parameter("buffer_size", "1000"));
        
        // 統計初期化
        total_games_ = 0;
        total_reward_ = 0.0;
        
        // モデルファイルパス
        model_file_path_ = get_parameter("model_file", "rl_model.dat");
    }
    
    bool initialize() override {
        if (!AIBase::initialize()) {
            return false;
        }
        
        // 保存されたモデルの読み込み
        load_model();
        
        return true;
    }
    
    void shutdown() override {
        // モデルの保存
        save_model();
        AIBase::shutdown();
    }
    
    AIDecision think(const GameState& state) override {
        if (!initialized_) {
            return AIDecision(-1, 0, {}, 0.0, "AI not initialized");
        }
        
        if (!state.own_field) {
            return AIDecision(-1, 0, {}, 0.0, "Field not available");
        }
        
        // 状態をエンコード
        RLState rl_state = encode_state(*state.own_field, state.current_pair);
        
        // Q学習による行動選択
        auto action = select_action(rl_state, *state.own_field);
        
        if (action.first == -1 || action.second == -1) {
            return AIDecision(-1, 0, {}, 0.0, "No valid actions available");
        }
        
        // MoveCommandリストを生成
        auto move_commands = MoveCommandGenerator::generate_move_commands(
            *state.own_field, action.first, action.second);
        
        // Q値から確信度を計算
        double confidence = calculate_confidence(rl_state, action);
        
        std::string reason = "RL Q-Learning: epsilon=" + std::to_string(epsilon_) + 
                           " Q=" + std::to_string(get_q_value(rl_state, action)) +
                           " at (" + std::to_string(action.first) + 
                           ", " + rotation_to_string(action.second) + ")";
        
        return AIDecision(action.first, action.second, move_commands, confidence, reason);
    }
    
    std::string get_type() const override {
        return "RLPlayer";
    }
    
    std::string get_debug_info() const override {
        return "RLPlayerAI lr=" + std::to_string(learning_rate_) + 
               " eps=" + std::to_string(epsilon_) + 
               " games=" + std::to_string(total_games_) +
               " avg_reward=" + std::to_string(get_average_reward());
    }
    
    int get_think_time_ms() const override {
        return 200; // 200msの思考時間制限
    }
    
    // 学習用メソッド
    void add_experience(const RLState& state, const std::pair<int, int>& action,
                       double reward, const RLState& next_state, bool is_terminal) {
        experience_buffer_.emplace_back(state, action, reward, next_state, is_terminal);
        
        // バッファサイズ制限
        if (experience_buffer_.size() > static_cast<size_t>(max_buffer_size_)) {
            experience_buffer_.erase(experience_buffer_.begin());
        }
        
        // 学習実行
        learn_from_experience();
        
        // 統計更新
        total_reward_ += reward;
        if (is_terminal) {
            total_games_++;
            reward_history_.push_back(total_reward_);
            total_reward_ = 0.0; // 次のエピソードのためリセット
            
            // εの減衰
            if (epsilon_ > min_epsilon_) {
                epsilon_ *= epsilon_decay_;
            }
        }
    }
    
    // 外部からの学習データ提供
    void provide_feedback(const GameState& state, const std::pair<int, int>& action, double score) {
        if (!state.own_field) return;
        
        RLState rl_state = encode_state(*state.own_field, state.current_pair);
        
        // スコアを報酬に変換（正規化）
        double reward = std::min(100.0, score / 10.0); // スコアを10で割って上限100に制限
        
        // 簡易的な次状態（終端状態として扱う）
        RLState dummy_next_state;
        add_experience(rl_state, action, reward, dummy_next_state, true);
    }

private:
    // 状態エンコーディング
    RLState encode_state(const Field& field, const PuyoPair& current_pair) {
        RLState state;
        
        // フィールド状態のエンコード（簡易版：各セルの色）
        for (int y = 0; y < FIELD_HEIGHT; ++y) {
            for (int x = 0; x < FIELD_WIDTH; ++x) {
                PuyoColor color = field.get_puyo(Position(x, y));
                state.field_state[y * FIELD_WIDTH + x] = static_cast<int>(color);
            }
        }
        
        // 現在のぷよペア色
        state.current_colors[0] = static_cast<int>(current_pair.axis);
        state.current_colors[1] = static_cast<int>(current_pair.child);
        
        return state;
    }
    
    // 状態のハッシュ化（Q値テーブルのキー用）
    std::string hash_state(const RLState& state) {
        std::string hash_str;
        
        // フィールド状態の圧縮表現（高さのみ）
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            int height = 0;
            for (int y = FIELD_HEIGHT - 1; y >= 0; --y) {
                if (state.field_state[y * FIELD_WIDTH + x] != 0) {
                    height = FIELD_HEIGHT - y;
                    break;
                }
            }
            hash_str += std::to_string(height) + ",";
        }
        
        // 現在のぷよペア色を追加
        hash_str += std::to_string(state.current_colors[0]) + "," + 
                   std::to_string(state.current_colors[1]);
        
        return hash_str;
    }
    
    // 行動選択（ε-greedy戦略）
    std::pair<int, int> select_action(const RLState& state, const Field& field) {
        std::vector<std::pair<int, int>> valid_actions = get_valid_actions(field);
        
        if (valid_actions.empty()) {
            return {-1, -1};
        }
        
        // ε-greedy戦略
        if (uniform_dist_(gen_) < epsilon_) {
            // 探索：ランダム行動
            std::uniform_int_distribution<> action_dist(0, valid_actions.size() - 1);
            return valid_actions[action_dist(gen_)];
        } else {
            // 活用：最良Q値の行動
            std::pair<int, int> best_action = valid_actions[0];
            double best_q_value = get_q_value(state, best_action);
            
            for (const auto& action : valid_actions) {
                double q_value = get_q_value(state, action);
                if (q_value > best_q_value) {
                    best_q_value = q_value;
                    best_action = action;
                }
            }
            
            return best_action;
        }
    }
    
    // 有効な行動の取得
    std::vector<std::pair<int, int>> get_valid_actions(const Field& field) {
        std::vector<std::pair<int, int>> actions;
        
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            for (int r = 0; r < 4; ++r) {
                if (field.can_place(x, r)) {
                    actions.push_back({x, r});
                }
            }
        }
        
        return actions;
    }
    
    // Q値の取得
    double get_q_value(const RLState& state, const std::pair<int, int>& action) {
        std::string state_hash = hash_state(state);
        
        auto state_it = q_table_.find(state_hash);
        if (state_it == q_table_.end()) {
            return 0.0; // 未知の状態は0で初期化
        }
        
        auto action_it = state_it->second.find(action);
        if (action_it == state_it->second.end()) {
            return 0.0; // 未知の行動は0で初期化
        }
        
        return action_it->second.q_value;
    }
    
    // Q値の更新
    void update_q_value(const RLState& state, const std::pair<int, int>& action, double new_q_value) {
        std::string state_hash = hash_state(state);
        
        auto& entry = q_table_[state_hash][action];
        entry.q_value = new_q_value;
        entry.visit_count++;
    }
    
    // 経験からの学習
    void learn_from_experience() {
        if (experience_buffer_.empty()) return;
        
        // 最新の経験から学習
        const Experience& exp = experience_buffer_.back();
        
        double current_q = get_q_value(exp.state, exp.action);
        double next_max_q = 0.0;
        
        if (!exp.is_terminal) {
            // 次状態の最大Q値を計算
            std::vector<std::pair<int, int>> next_actions = {{0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {5,0}}; // 簡易版
            for (const auto& next_action : next_actions) {
                double next_q = get_q_value(exp.next_state, next_action);
                next_max_q = std::max(next_max_q, next_q);
            }
        }
        
        // Q学習の更新式
        double target = exp.reward + discount_factor_ * next_max_q;
        double new_q = current_q + learning_rate_ * (target - current_q);
        
        update_q_value(exp.state, exp.action, new_q);
    }
    
    // 確信度計算
    double calculate_confidence(const RLState& state, const std::pair<int, int>& action) {
        double q_value = get_q_value(state, action);
        std::string state_hash = hash_state(state);
        
        // 訪問回数を考慮した確信度
        auto state_it = q_table_.find(state_hash);
        if (state_it != q_table_.end()) {
            auto action_it = state_it->second.find(action);
            if (action_it != state_it->second.end()) {
                int visit_count = action_it->second.visit_count;
                double confidence = std::tanh(q_value / 10.0) * 0.5 + 0.5; // Q値の正規化
                confidence = std::min(1.0, confidence + visit_count * 0.01); // 訪問回数ボーナス
                return confidence;
            }
        }
        
        return 0.1; // デフォルト確信度
    }
    
    // 平均報酬の計算
    double get_average_reward() const {
        if (reward_history_.empty()) return 0.0;
        
        double sum = 0.0;
        for (double reward : reward_history_) {
            sum += reward;
        }
        return sum / reward_history_.size();
    }
    
    // モデルの保存
    void save_model() {
        std::ofstream file(model_file_path_, std::ios::binary);
        if (!file.is_open()) return;
        
        // Q値テーブルのサイズ
        size_t table_size = q_table_.size();
        file.write(reinterpret_cast<const char*>(&table_size), sizeof(table_size));
        
        // 各状態のQ値を保存
        for (const auto& state_pair : q_table_) {
            const std::string& state_hash = state_pair.first;
            const auto& actions = state_pair.second;
            
            // 状態ハッシュの長さと内容
            size_t hash_len = state_hash.length();
            file.write(reinterpret_cast<const char*>(&hash_len), sizeof(hash_len));
            file.write(state_hash.c_str(), hash_len);
            
            // アクションの数
            size_t action_count = actions.size();
            file.write(reinterpret_cast<const char*>(&action_count), sizeof(action_count));
            
            // 各アクションのQ値
            for (const auto& action_pair : actions) {
                const auto& action = action_pair.first;
                const QEntry& entry = action_pair.second;
                
                file.write(reinterpret_cast<const char*>(&action.first), sizeof(action.first));
                file.write(reinterpret_cast<const char*>(&action.second), sizeof(action.second));
                file.write(reinterpret_cast<const char*>(&entry.q_value), sizeof(entry.q_value));
                file.write(reinterpret_cast<const char*>(&entry.visit_count), sizeof(entry.visit_count));
            }
        }
        
        file.close();
    }
    
    // モデルの読み込み
    void load_model() {
        std::ifstream file(model_file_path_, std::ios::binary);
        if (!file.is_open()) return;
        
        q_table_.clear();
        
        // Q値テーブルのサイズ
        size_t table_size;
        file.read(reinterpret_cast<char*>(&table_size), sizeof(table_size));
        
        for (size_t i = 0; i < table_size; ++i) {
            // 状態ハッシュ
            size_t hash_len;
            file.read(reinterpret_cast<char*>(&hash_len), sizeof(hash_len));
            
            std::string state_hash(hash_len, '\0');
            file.read(&state_hash[0], hash_len);
            
            // アクションの数
            size_t action_count;
            file.read(reinterpret_cast<char*>(&action_count), sizeof(action_count));
            
            // 各アクションのQ値
            for (size_t j = 0; j < action_count; ++j) {
                std::pair<int, int> action;
                QEntry entry;
                
                file.read(reinterpret_cast<char*>(&action.first), sizeof(action.first));
                file.read(reinterpret_cast<char*>(&action.second), sizeof(action.second));
                file.read(reinterpret_cast<char*>(&entry.q_value), sizeof(entry.q_value));
                file.read(reinterpret_cast<char*>(&entry.visit_count), sizeof(entry.visit_count));
                
                q_table_[state_hash][action] = entry;
            }
        }
        
        file.close();
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