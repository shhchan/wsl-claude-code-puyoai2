#pragma once

#include "ai_base.h"
#include "ai_utils.h"
#include "core/field.h"
#include <vector>
#include <memory>
#include <random>
#include <map>
#include <fstream>
#include <algorithm>
#include <cmath>

namespace puyo {
namespace ai {

// 改良された人間プレイデータ記録
struct HumanPlayData {
    std::vector<int> field_state;      // フィールド状態のエンコーディング
    int current_colors[2];             // 現在のぷよペア色
    std::vector<int> next_colors;      // ネクスト情報
    std::pair<int, int> action;        // 選択した行動 (x, r)
    double confidence;                 // 人間の判断への信頼度
    int game_turn;                     // ゲーム内のターン数
    int chain_context;                 // 連鎖文脈（0:構築中, 1:発火, 2:継続）
    double field_stability;            // フィールド安定性スコア
    int height_profile[FIELD_WIDTH];   // 各列の高さ
    
    HumanPlayData() : field_state(FIELD_WIDTH * FIELD_HEIGHT, 0), next_colors(4, 0) {
        current_colors[0] = 0;
        current_colors[1] = 0;
        action = {-1, -1};
        confidence = 1.0;
        game_turn = 0;
        chain_context = 0;
        field_stability = 0.0;
        for (int i = 0; i < FIELD_WIDTH; ++i) height_profile[i] = 0;
    }
    
    // フィールド状態の類似度計算
    double calculate_field_similarity(const HumanPlayData& other) const {
        double similarity = 0.0;
        int total_comparisons = 0;
        
        // 高さプロファイルの類似度
        double height_similarity = 0.0;
        for (int i = 0; i < FIELD_WIDTH; ++i) {
            int diff = std::abs(height_profile[i] - other.height_profile[i]);
            height_similarity += std::max(0.0, 1.0 - diff / 5.0); // 5段差まで許容
        }
        height_similarity /= FIELD_WIDTH;
        
        // 色分布の類似度（簡易版）
        double color_similarity = 0.0;
        if (current_colors[0] == other.current_colors[0] && 
            current_colors[1] == other.current_colors[1]) {
            color_similarity = 1.0;
        } else if (current_colors[0] == other.current_colors[1] &&
                  current_colors[1] == other.current_colors[0]) {
            color_similarity = 0.8; // 順序違いは許容
        }
        
        // 総合類似度
        similarity = height_similarity * 0.7 + color_similarity * 0.3;
        
        return similarity;
    }
};

// 状況の類似度計算用
struct SituationSimilarity {
    int data_index;
    double similarity_score;
    
    SituationSimilarity() : data_index(-1), similarity_score(0.0) {}
    SituationSimilarity(int idx, double score) 
        : data_index(idx), similarity_score(score) {}
};

// 改良された人間行動学習AI
class HumanLearningAI : public AIBase {
private:
    // YAML設定
    struct LearningConfig {
        std::string data_dir;
        int min_games_required;
        double quality_threshold;
        int max_chain_filter;
        double field_similarity_threshold;
        double action_confidence_threshold;
        double imitation_strength;
        double randomness_factor;
        double confidence_bonus;
        
        LearningConfig() : data_dir("data/human_play"), min_games_required(100),
                          quality_threshold(0.7), max_chain_filter(3),
                          field_similarity_threshold(0.8), action_confidence_threshold(0.6),
                          imitation_strength(0.8), randomness_factor(0.1), confidence_bonus(0.1) {}
    } config_;
    
    // 重み設定
    struct WeightFactors {
        double field_structure;
        double color_distribution;
        double height_profile;
        double next_compatibility;
        
        WeightFactors() : field_structure(0.4), color_distribution(0.3),
                         height_profile(0.2), next_compatibility(0.1) {}
    } weights_;
    
    // 学習データベース（改良版）
    std::vector<HumanPlayData> learning_database_;
    std::map<std::string, std::vector<int>> pattern_clusters_;  // パターンクラスタリング
    
    // 類似度計算とマッチング
    double min_similarity_threshold_;
    int search_depth_;
    
    // ランダム生成器
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_real_distribution<> uniform_dist_;
    
    // 性能監視
    struct PerformanceStats {
        int total_queries;
        int successful_matches;
        int exact_matches;
        int similar_matches;
        int fallback_uses;
        double avg_similarity;
        double avg_confidence;
        
        PerformanceStats() : total_queries(0), successful_matches(0), 
                           exact_matches(0), similar_matches(0), fallback_uses(0),
                           avg_similarity(0.0), avg_confidence(0.0) {}
    } stats_;
    
    // モデル管理
    std::string model_save_path_;
    bool online_learning_enabled_;
    int memory_limit_;
    
public:
    HumanLearningAI(const AIParameters& params = {}) 
        : AIBase("HumanLearningAI"), gen_(rd_()), uniform_dist_(0.0, 1.0),
          min_similarity_threshold_(0.7), search_depth_(5),
          online_learning_enabled_(true), memory_limit_(5000) {
        
        // YAML設定の読み込み
        load_configuration();
        
        // パラメータの設定
        for (const auto& param : params) {
            set_parameter(param.first, param.second);
        }
        
        // デフォルト設定
        field_weight_ = std::stod(get_parameter("field_weight", "0.7"));
        color_weight_ = std::stod(get_parameter("color_weight", "0.2"));
        turn_weight_ = std::stod(get_parameter("turn_weight", "0.1"));
        
        max_similar_cases_ = std::stoi(get_parameter("max_similar_cases", "5"));
        min_similarity_threshold_ = std::stod(get_parameter("min_similarity", "0.3"));
        random_fallback_rate_ = std::stod(get_parameter("random_fallback", "0.1"));
        
        // データファイル設定
        data_file_path_ = get_parameter("data_file", "human_play_data.dat");
        auto_save_enabled_ = (get_parameter("auto_save", "true") == "true");
        
        // 統計初期化
        total_queries_ = 0;
        successful_matches_ = 0;
        avg_similarity_ = 0.0;
    }
    
    bool initialize() override {
        if (!AIBase::initialize()) {
            return false;
        }
        
        // 学習データの読み込み
        load_learning_data();
        
        // デモデータがない場合は基本的なパターンを生成
        if (learning_database_.empty()) {
            generate_demo_patterns();
        }
        
        return true;
    }
    
    void shutdown() override {
        // 学習データの保存
        if (auto_save_enabled_) {
            save_learning_data();
        }
        AIBase::shutdown();
    }
    
    AIDecision think(const GameState& state) override {
        if (!initialized_) {
            return AIDecision(-1, 0, {}, 0.0, "AI not initialized");
        }
        
        if (!state.own_field) {
            return AIDecision(-1, 0, {}, 0.0, "Field not available");
        }
        
        total_queries_++;
        
        // 現在の状況をエンコード
        HumanPlayData current_situation;
        encode_situation(current_situation, *state.own_field, state.current_pair, state.turn_count);
        
        // 類似状況を検索
        std::vector<SituationSimilarity> similar_cases = find_similar_situations(current_situation);
        
        std::pair<int, int> selected_action;
        double confidence;
        std::string reason;
        
        if (!similar_cases.empty() && similar_cases[0].similarity_score >= min_similarity_threshold_) {
            // 類似ケースに基づく行動選択
            selected_action = select_action_from_similar_cases(similar_cases);
            confidence = calculate_confidence_from_similarity(similar_cases);
            
            successful_matches_++;
            avg_similarity_ = (avg_similarity_ * (successful_matches_ - 1) + similar_cases[0].similarity_score) / successful_matches_;
            
            reason = "Human behavior imitation: similarity=" + 
                    std::to_string(similar_cases[0].similarity_score) + 
                    " from " + std::to_string(similar_cases.size()) + " cases";
        } else {
            // フォールバック：ランダム行動またはデフォルト戦略
            if (uniform_dist_(gen_) < random_fallback_rate_) {
                selected_action = select_random_action(*state.own_field);
                confidence = 0.2;
                reason = "Fallback: random action (no similar cases)";
            } else {
                selected_action = select_default_strategy_action(*state.own_field);
                confidence = 0.4;
                reason = "Fallback: default strategy (no similar cases)";
            }
        }
        
        // 有効な行動かチェック
        if (selected_action.first == -1 || selected_action.second == -1) {
            selected_action = select_safe_action(*state.own_field);
            confidence = 0.1;
            reason = "Emergency fallback: safe action";
        }
        
        // MoveCommandリストを生成
        auto move_commands = MoveCommandGenerator::generate_move_commands(
            *state.own_field, selected_action.first, selected_action.second);
        
        return AIDecision(selected_action.first, selected_action.second, 
                         move_commands, confidence, reason);
    }
    
    std::string get_type() const override {
        return "HumanLearning";
    }
    
    std::string get_debug_info() const override {
        double success_rate = (total_queries_ > 0) ? 
                             (double)successful_matches_ / total_queries_ * 100.0 : 0.0;
        
        return "HumanLearningAI data=" + std::to_string(learning_database_.size()) + 
               " success_rate=" + std::to_string(success_rate) + "%" +
               " avg_sim=" + std::to_string(avg_similarity_);
    }
    
    int get_think_time_ms() const override {
        return 150; // 150msの思考時間制限
    }
    
    // 人間プレイデータの追加
    void add_human_play_data(const Field& field, const PuyoPair& pair, 
                            int turn_count, int x, int r, double confidence = 1.0) {
        HumanPlayData data;
        encode_situation(data, field, pair, turn_count);
        data.action = {x, r};
        data.confidence = confidence;
        
        learning_database_.push_back(data);
        
        // データサイズ制限（メモリ管理）
        const size_t max_data_size = 10000;
        if (learning_database_.size() > max_data_size) {
            // 古いデータを削除（FIFO）
            learning_database_.erase(learning_database_.begin(), 
                                   learning_database_.begin() + (learning_database_.size() - max_data_size));
        }
    }
    
    // 学習データベースのサイズ取得
    size_t get_database_size() const {
        return learning_database_.size();
    }
    
    // 統計情報取得
    double get_success_rate() const {
        return (total_queries_ > 0) ? (double)successful_matches_ / total_queries_ : 0.0;
    }

private:
    // 状況エンコーディング
    void encode_situation(HumanPlayData& data, const Field& field, 
                         const PuyoPair& pair, int turn_count) {
        // フィールド状態の簡易エンコード（各列の高さ）
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            int height = 0;
            for (int y = FIELD_HEIGHT - 1; y >= 0; --y) {
                if (field.get_puyo(Position(x, y)) != PuyoColor::EMPTY) {
                    height = FIELD_HEIGHT - y;
                    break;
                }
            }
            data.field_state[x] = height;
        }
        
        // 色情報の分布もエンコード（簡易版）
        std::map<PuyoColor, int> color_counts;
        for (int y = 0; y < FIELD_HEIGHT; ++y) {
            for (int x = 0; x < FIELD_WIDTH; ++x) {
                PuyoColor color = field.get_puyo(Position(x, y));
                if (color != PuyoColor::EMPTY) {
                    color_counts[color]++;
                }
            }
        }
        
        // 追加情報として色分布を格納
        int color_idx = FIELD_WIDTH;
        for (auto& pair_count : color_counts) {
            if (color_idx < static_cast<int>(data.field_state.size())) {
                data.field_state[color_idx++] = pair_count.second;
            }
        }
        
        // 現在のぷよペア
        data.current_colors[0] = static_cast<int>(pair.axis);
        data.current_colors[1] = static_cast<int>(pair.child);
        data.game_turn = turn_count;
    }
    
    // 類似状況の検索
    std::vector<SituationSimilarity> find_similar_situations(const HumanPlayData& current) {
        std::vector<SituationSimilarity> similarities;
        
        for (size_t i = 0; i < learning_database_.size(); ++i) {
            const HumanPlayData& stored = learning_database_[i];
            double similarity = calculate_similarity(current, stored);
            
            if (similarity >= min_similarity_threshold_) {
                similarities.emplace_back(i, similarity);
            }
        }
        
        // 類似度の高い順にソート
        std::sort(similarities.begin(), similarities.end(),
                 [](const SituationSimilarity& a, const SituationSimilarity& b) {
                     return a.similarity_score > b.similarity_score;
                 });
        
        // 上位N個のみ返す
        if (similarities.size() > static_cast<size_t>(max_similar_cases_)) {
            similarities.resize(max_similar_cases_);
        }
        
        return similarities;
    }
    
    // 類似度計算
    double calculate_similarity(const HumanPlayData& a, const HumanPlayData& b) {
        double similarity = 0.0;
        
        // フィールド状態の類似度（各列の高さ比較）
        double field_sim = 0.0;
        for (int i = 0; i < FIELD_WIDTH; ++i) {
            int diff = std::abs(a.field_state[i] - b.field_state[i]);
            field_sim += (FIELD_HEIGHT - diff) / static_cast<double>(FIELD_HEIGHT);
        }
        field_sim /= FIELD_WIDTH;
        
        // 色の類似度
        double color_sim = 0.0;
        if (a.current_colors[0] == b.current_colors[0]) color_sim += 0.5;
        if (a.current_colors[1] == b.current_colors[1]) color_sim += 0.5;
        
        // ターン数の類似度
        int turn_diff = std::abs(a.game_turn - b.game_turn);
        double turn_sim = std::max(0.0, 1.0 - turn_diff / 20.0); // 20ターン差で類似度0
        
        // 重み付け合計
        similarity = field_sim * field_weight_ + 
                    color_sim * color_weight_ + 
                    turn_sim * turn_weight_;
        
        return similarity;
    }
    
    // 類似ケースから行動選択
    std::pair<int, int> select_action_from_similar_cases(const std::vector<SituationSimilarity>& cases) {
        if (cases.empty()) {
            return {-1, -1};
        }
        
        // 重み付けランダム選択（類似度が高いほど選ばれやすい）
        std::vector<double> weights;
        for (const auto& case_sim : cases) {
            const HumanPlayData& data = learning_database_[case_sim.data_index];
            double weight = case_sim.similarity_score * data.confidence;
            weights.push_back(weight);
        }
        
        // 重み付け選択
        std::discrete_distribution<> dist(weights.begin(), weights.end());
        int selected_idx = dist(gen_);
        
        const HumanPlayData& selected_data = learning_database_[cases[selected_idx].data_index];
        return selected_data.action;
    }
    
    // 類似度から確信度を計算
    double calculate_confidence_from_similarity(const std::vector<SituationSimilarity>& cases) {
        if (cases.empty()) {
            return 0.1;
        }
        
        // 最高類似度と類似ケース数を考慮
        double max_similarity = cases[0].similarity_score;
        int case_count = std::min(static_cast<int>(cases.size()), 3);
        
        double confidence = max_similarity * 0.7 + (case_count / 5.0) * 0.3;
        return std::min(1.0, confidence);
    }
    
    // ランダム行動選択
    std::pair<int, int> select_random_action(const Field& field) {
        std::vector<std::pair<int, int>> valid_actions;
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            for (int r = 0; r < 4; ++r) {
                if (field.can_place(x, r)) {
                    valid_actions.push_back({x, r});
                }
            }
        }
        
        if (valid_actions.empty()) {
            return {-1, -1};
        }
        
        std::uniform_int_distribution<> dist(0, valid_actions.size() - 1);
        return valid_actions[dist(gen_)];
    }
    
    // デフォルト戦略行動選択（中央寄せ戦略）
    std::pair<int, int> select_default_strategy_action(const Field& field) {
        // 中央から順に配置可能性をチェック
        int center = FIELD_WIDTH / 2;
        for (int offset = 0; offset < FIELD_WIDTH; ++offset) {
            for (int direction : {0, 1}) { // 左右対称にチェック
                int x = center + (direction == 0 ? -offset : offset);
                if (x < 0 || x >= FIELD_WIDTH) continue;
                
                for (int r : {0, 1, 2, 3}) { // 回転優先度
                    if (field.can_place(x, r)) {
                        return {x, r};
                    }
                }
            }
        }
        
        return {-1, -1};
    }
    
    // 安全行動選択（最後の手段）
    std::pair<int, int> select_safe_action(const Field& field) {
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            for (int r = 0; r < 4; ++r) {
                if (field.can_place(x, r)) {
                    return {x, r};
                }
            }
        }
        return {2, 0}; // 中央、上向き
    }
    
    // デモパターンの生成
    void generate_demo_patterns() {
        // 基本的な人間らしい行動パターンを生成
        // 1. 中央寄せ戦略
        for (int x = 1; x <= 4; ++x) {
            HumanPlayData demo_data;
            demo_data.current_colors[0] = static_cast<int>(PuyoColor::RED);
            demo_data.current_colors[1] = static_cast<int>(PuyoColor::BLUE);
            demo_data.action = {x, 0};
            demo_data.confidence = 0.8;
            demo_data.game_turn = 1;
            learning_database_.push_back(demo_data);
        }
        
        // 2. 端の回避パターン
        HumanPlayData avoid_edge;
        avoid_edge.current_colors[0] = static_cast<int>(PuyoColor::GREEN);
        avoid_edge.current_colors[1] = static_cast<int>(PuyoColor::YELLOW);
        avoid_edge.action = {2, 0}; // 中央寄り
        avoid_edge.confidence = 0.7;
        avoid_edge.game_turn = 5;
        learning_database_.push_back(avoid_edge);
    }
    
    // 学習データの保存
    void save_learning_data() {
        std::ofstream file(data_file_path_, std::ios::binary);
        if (!file.is_open()) return;
        
        // データ数
        size_t data_count = learning_database_.size();
        file.write(reinterpret_cast<const char*>(&data_count), sizeof(data_count));
        
        // 各データの保存
        for (const HumanPlayData& data : learning_database_) {
            file.write(reinterpret_cast<const char*>(data.field_state.data()), 
                      data.field_state.size() * sizeof(int));
            file.write(reinterpret_cast<const char*>(data.current_colors), 
                      2 * sizeof(int));
            file.write(reinterpret_cast<const char*>(&data.action.first), 
                      sizeof(data.action.first));
            file.write(reinterpret_cast<const char*>(&data.action.second), 
                      sizeof(data.action.second));
            file.write(reinterpret_cast<const char*>(&data.confidence), 
                      sizeof(data.confidence));
            file.write(reinterpret_cast<const char*>(&data.game_turn), 
                      sizeof(data.game_turn));
        }
        
        file.close();
    }
    
    // 学習データの読み込み
    void load_learning_data() {
        std::ifstream file(data_file_path_, std::ios::binary);
        if (!file.is_open()) return;
        
        learning_database_.clear();
        
        // データ数
        size_t data_count;
        file.read(reinterpret_cast<char*>(&data_count), sizeof(data_count));
        
        // 各データの読み込み
        for (size_t i = 0; i < data_count; ++i) {
            HumanPlayData data;
            
            file.read(reinterpret_cast<char*>(data.field_state.data()), 
                     data.field_state.size() * sizeof(int));
            file.read(reinterpret_cast<char*>(data.current_colors), 
                     2 * sizeof(int));
            file.read(reinterpret_cast<char*>(&data.action.first), 
                     sizeof(data.action.first));
            file.read(reinterpret_cast<char*>(&data.action.second), 
                     sizeof(data.action.second));
            file.read(reinterpret_cast<char*>(&data.confidence), 
                     sizeof(data.confidence));
            file.read(reinterpret_cast<char*>(&data.game_turn), 
                     sizeof(data.game_turn));
            
            learning_database_.push_back(data);
        }
        
        file.close();
    }
};

} // namespace ai
} // namespace puyo