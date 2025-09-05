#pragma once

#include "ai_base.h"
#include "ai_utils.h"
#include "core/field.h"
#include <vector>
#include <memory>
#include <climits>
#include <map>
#include <algorithm>
#include <chrono>

namespace puyo {
namespace ai {

// 高度な連鎖探索AI（U字型構築・ネクスト対応・YAML設定対応）
class ChainSearchAI : public AIBase {
private:
    // 設定パラメータ
    int search_depth_;
    int think_time_limit_;
    
    // 評価関数の重み（YAML設定から読み込み）
    struct EvaluationWeights {
        double chain_potential;
        double chain_trigger;
        double next_compatibility;
        double u_shape_bonus;
        double center_preference;
        double height_balance;
        double stability;
        double color_grouping;
        double color_balance;
        double height_penalty;
        double gameover_penalty;
        
        EvaluationWeights() : chain_potential(15.0), chain_trigger(25.0), 
                             next_compatibility(8.0), u_shape_bonus(12.0),
                             center_preference(3.0), height_balance(4.0),
                             stability(6.0), color_grouping(10.0),
                             color_balance(2.0), height_penalty(-20.0),
                             gameover_penalty(-100.0) {}
    } weights_;
    
    // U字型評価設定
    struct UShapeConfig {
        std::vector<int> preferred_columns;
        std::vector<int> center_columns;
        int ideal_height_diff;
        int max_center_height;
        
        UShapeConfig() : preferred_columns({1, 2, 4, 5}), center_columns({2, 3}),
                        ideal_height_diff(3), max_center_height(8) {}
    } u_config_;
    
    // 連鎖戦略設定
    struct ChainStrategy {
        int min_chain_target;
        bool multi_color_chains;
        int chain_timing_threshold;
        
        ChainStrategy() : min_chain_target(3), multi_color_chains(true),
                         chain_timing_threshold(8) {}
    } chain_strategy_;
    
    // デバッグ設定
    bool verbose_evaluation_;
    bool show_position_scores_;
    bool log_chain_analysis_;
    
public:
    ChainSearchAI(const AIParameters& params = {}) 
        : AIBase("ChainSearchAI"), verbose_evaluation_(false), 
          show_position_scores_(false), log_chain_analysis_(true) {
        
        // パラメータの設定
        for (const auto& param : params) {
            set_parameter(param.first, param.second);
        }
        
        // YAML設定ファイルから設定を読み込み
        load_configuration();
    }
    
    bool initialize() override {
        if (log_chain_analysis_) {
            // 初期化ログ
        }
        return AIBase::initialize();
    }
    
    // YAML設定ファイル読み込み
    void load_configuration() {
        std::string config_path = "config/ai_params/chain_search.yaml";
        auto config = ConfigLoader::load_config(config_path);
        
        // 基本パラメータ
        search_depth_ = ConfigLoader::get_int(config, "search_depth", 4);
        think_time_limit_ = ConfigLoader::get_int(config, "think_time_limit", 400);
        
        // 探索深度制限
        search_depth_ = std::max(1, std::min(search_depth_, 7));
        
        // 評価関数重み
        weights_.chain_potential = ConfigLoader::get_double(config, "evaluation_weights.chain_potential", 15.0);
        weights_.chain_trigger = ConfigLoader::get_double(config, "evaluation_weights.chain_trigger", 25.0);
        weights_.next_compatibility = ConfigLoader::get_double(config, "evaluation_weights.next_compatibility", 8.0);
        weights_.u_shape_bonus = ConfigLoader::get_double(config, "evaluation_weights.u_shape_bonus", 12.0);
        weights_.center_preference = ConfigLoader::get_double(config, "evaluation_weights.center_preference", 3.0);
        weights_.height_balance = ConfigLoader::get_double(config, "evaluation_weights.height_balance", 4.0);
        weights_.stability = ConfigLoader::get_double(config, "evaluation_weights.stability", 6.0);
        weights_.color_grouping = ConfigLoader::get_double(config, "evaluation_weights.color_grouping", 10.0);
        weights_.color_balance = ConfigLoader::get_double(config, "evaluation_weights.color_balance", 2.0);
        weights_.height_penalty = ConfigLoader::get_double(config, "evaluation_weights.height_penalty", -20.0);
        weights_.gameover_penalty = ConfigLoader::get_double(config, "evaluation_weights.gameover_penalty", -100.0);
        
        // U字型設定
        u_config_.ideal_height_diff = ConfigLoader::get_int(config, "u_shape_evaluation.ideal_height_diff", 3);
        u_config_.max_center_height = ConfigLoader::get_int(config, "u_shape_evaluation.max_center_height", 8);
        
        // 連鎖戦略
        chain_strategy_.min_chain_target = ConfigLoader::get_int(config, "chain_strategy.min_chain_target", 3);
        chain_strategy_.multi_color_chains = ConfigLoader::get_bool(config, "chain_strategy.multi_color_chains", true);
        chain_strategy_.chain_timing_threshold = ConfigLoader::get_int(config, "chain_strategy.chain_timing_threshold", 8);
        
        // デバッグ設定
        verbose_evaluation_ = ConfigLoader::get_bool(config, "debug.verbose_evaluation", false);
        show_position_scores_ = ConfigLoader::get_bool(config, "debug.show_position_scores", false);
        log_chain_analysis_ = ConfigLoader::get_bool(config, "debug.log_chain_analysis", true);
    }
    
    AIDecision think(const GameState& state) override {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (!initialized_) {
            return AIDecision(-1, 0, {}, 0.0, "AI not initialized");
        }
        
        if (!state.own_field) {
            return AIDecision(-1, 0, {}, 0.0, "Field not available");
        }
        
        // フィールド分析情報を更新
        auto analysis = calculate_field_analysis(*state.own_field);
        
        // 配置可能な全位置を取得
        auto valid_positions = get_all_valid_positions(*state.own_field);
        
        if (valid_positions.empty()) {
            return AIDecision(-1, 0, {}, 0.0, "No valid positions available");
        }
        
        // 高度な評価による最良手選択
        std::pair<int, int> best_move = {-1, -1};
        double best_score = -std::numeric_limits<double>::max();
        std::string best_reason = "";
        
        for (const auto& pos : valid_positions) {
            // 時間制限チェック
            auto current_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
            if (elapsed.count() > think_time_limit_) {
                break; // 時間切れ
            }
            
            // 高度な評価関数による評価
            auto eval_result = evaluate_position_advanced(*state.own_field, pos.first, pos.second, state);
            
            if (eval_result.total_score > best_score) {
                best_score = eval_result.total_score;
                best_move = pos;
                best_reason = eval_result.reason;
            }
            
            if (show_position_scores_) {
                // デバッグ用位置スコア表示
            }
        }
        
        if (best_move.first == -1) {
            // フォールバック：中央寄り位置を選択
            best_move = select_fallback_position(valid_positions);
        }
        
        // MoveCommandリストを生成
        auto move_commands = MoveCommandGenerator::generate_move_commands(
            *state.own_field, best_move.first, best_move.second);
        
        // 確信度を計算（より洗練された計算）
        double confidence = calculate_confidence(best_score, analysis);
        
        // 思考時間を計算
        auto end_time = std::chrono::high_resolution_clock::now();
        auto think_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::string reason = "ChainSearch[depth=" + std::to_string(search_depth_) + 
                           ", score=" + std::to_string(best_score) + 
                           ", time=" + std::to_string(think_duration.count()) + "ms]: " + 
                           best_reason;
        
        return AIDecision(best_move.first, best_move.second, move_commands, confidence, reason);
    }
    
    std::string get_type() const override {
        return "ChainSearch";
    }
    
    std::string get_debug_info() const override {
        return "ChainSearchAI[depth=" + std::to_string(search_depth_) + 
               ", u_shape=" + std::to_string(weights_.u_shape_bonus) +
               ", chain=" + std::to_string(weights_.chain_potential) + "]";
    }
    
    int get_think_time_ms() const override {
        return think_time_limit_;
    }

private:
    // 評価結果構造体
    struct EvaluationResult {
        double total_score;
        double chain_score;
        double u_shape_score;
        double next_score;
        double stability_score;
        std::string reason;
        
        EvaluationResult() : total_score(0.0), chain_score(0.0), u_shape_score(0.0),
                           next_score(0.0), stability_score(0.0) {}
    };
    
    // 高度な位置評価関数（ネクスト情報・U字型・連鎖ポテンシャルを考慮）
    EvaluationResult evaluate_position_advanced(const Field& field, int x, int r, const GameState& state) {
        EvaluationResult result;
        
        // 1. 基本評価（中央寄り・高さ）
        double basic_score = evaluate_basic_position(field, x, r);
        result.total_score += basic_score;
        
        // 2. U字型評価
        result.u_shape_score = evaluate_u_shape_contribution(field, x, r);
        result.total_score += result.u_shape_score * weights_.u_shape_bonus;
        
        // 3. 連鎖ポテンシャル評価
        result.chain_score = evaluate_chain_potential_contribution(field, x, r);
        result.total_score += result.chain_score * weights_.chain_potential;
        
        // 4. ネクスト互換性評価（ネクスト情報を活用）
        if (!state.next_queue.empty()) {
            result.next_score = evaluate_with_next_info(field, x, r, state.next_queue);
            result.total_score += result.next_score * weights_.next_compatibility;
        }
        
        // 5. フィールド安定性
        result.stability_score = evaluate_field_stability(field, x);
        result.total_score += result.stability_score * weights_.stability;
        
        // 6. 色バランス評価
        double color_score = FieldAnalyzer::evaluate_color_balance(field) * weights_.color_balance;
        result.total_score += color_score;
        
        // 7. ゲームオーバー回避
        int height = get_column_height(field, x);
        if (height >= FIELD_HEIGHT - 2) {
            result.total_score += weights_.gameover_penalty;
        }
        
        // 8. 連鎖発火タイミング判定
        if (should_trigger_chain(field, state)) {
            double trigger_bonus = evaluate_chain_trigger_potential(field) * weights_.chain_trigger;
            result.total_score += trigger_bonus;
        }
        
        // 理由文字列の構築
        result.reason = build_evaluation_reason(result, x, r, height);
        
        return result;
    }
    
    // フォールバック位置選択（中央寄り）
    std::pair<int, int> select_fallback_position(const std::vector<std::pair<int, int>>& valid_positions) {
        std::pair<int, int> best = valid_positions[0];
        int best_center_score = 999;
        
        for (const auto& pos : valid_positions) {
            int center_distance = std::abs(pos.first - FIELD_WIDTH / 2);
            if (center_distance < best_center_score) {
                best_center_score = center_distance;
                best = pos;
            }
        }
        
        return best;
    }
    
    // 確信度計算
    double calculate_confidence(double score, const GameState::FieldAnalysis& analysis) {
        // スコアベースの基本確信度
        double base_confidence = std::tanh(score / 100.0) * 0.5 + 0.5;
        
        // フィールド安定性による調整
        double stability_factor = std::min(1.0, analysis.stability_score / 10.0);
        
        // 連鎖ポテンシャルによる調整
        double chain_factor = std::min(1.0, analysis.chain_potential / 20.0);
        
        double final_confidence = base_confidence * 0.6 + stability_factor * 0.2 + chain_factor * 0.2;
        return std::max(0.1, std::min(1.0, final_confidence));
    }
    
    // U字型貢献度評価
    double evaluate_u_shape_contribution(const Field& field, int x, int r) {
        // FieldAnalyzer::evaluate_u_shapeを使用して基本U字スコアを取得
        double base_u_score = FieldAnalyzer::evaluate_u_shape(field);
        
        // この配置がU字形成にどう貢献するかを評価
        double contribution = 0.0;
        
        // 設定された優先列への配置ボーナス
        if (std::find(u_config_.preferred_columns.begin(), 
                     u_config_.preferred_columns.end(), x) != u_config_.preferred_columns.end()) {
            contribution += 5.0;
        }
        
        // 中央列は低く保つ
        if (std::find(u_config_.center_columns.begin(),
                     u_config_.center_columns.end(), x) != u_config_.center_columns.end()) {
            int height = get_column_height(field, x);
            if (height < u_config_.max_center_height) {
                contribution += 3.0;
            } else {
                contribution -= 5.0; // 中央が高すぎる場合はペナルティ
            }
        }
        
        return base_u_score + contribution;
    }
    
    // 連鎖ポテンシャル貢献度評価
    double evaluate_chain_potential_contribution(const Field& field, int x, int r) {
        // 基本の連鎖ポテンシャルを取得
        int base_potential = FieldAnalyzer::count_potential_chains(field);
        
        // この手でどれだけ連鎖ポテンシャルが向上するかをシミュレーション
        // （実際の実装では配置シミュレーションが必要だが、簡易版として近似）
        double improvement = 0.0;
        
        // 同色ぷよとの隣接による連鎖構築貢献
        improvement += count_same_color_adjacency(field, x) * 2.0;
        
        return base_potential * 2.0 + improvement;
    }
    
    // フィールド安定性評価
    double evaluate_field_stability(const Field& field, int x) {
        std::vector<int> heights = get_all_column_heights(field);
        
        double stability = 0.0;
        
        // 高さの分散を評価（小さいほど安定）
        double mean_height = 0.0;
        for (int h : heights) mean_height += h;
        mean_height /= heights.size();
        
        double variance = 0.0;
        for (int h : heights) {
            variance += (h - mean_height) * (h - mean_height);
        }
        variance /= heights.size();
        
        stability = std::max(0.0, 10.0 - variance); // 分散が小さいほど高スコア
        
        return stability;
    }
    
    // 連鎖発火判定
    bool should_trigger_chain(const Field& field, const GameState& state) {
        // フィールドが一定以上の高さになった場合
        auto analysis = calculate_field_analysis(field);
        if (analysis.max_height >= chain_strategy_.chain_timing_threshold) {
            return true;
        }
        
        // 十分な連鎖ポテンシャルがある場合
        if (analysis.chain_potential >= chain_strategy_.min_chain_target * 10) {
            return true;
        }
        
        return false;
    }
    
    // 連鎖発火ポテンシャル評価
    double evaluate_chain_trigger_potential(const Field& field) {
        int potential_chains = FieldAnalyzer::count_potential_chains(field);
        return potential_chains * 10.0; // 連鎖可能数×10
    }
    
    // 評価理由文字列構築
    std::string build_evaluation_reason(const EvaluationResult& result, int x, int r, int height) {
        std::string reason = "(" + std::to_string(x) + "," + rotation_to_string(r) + ") ";
        
        if (result.u_shape_score > 5.0) reason += "U-shape+ ";
        if (result.chain_score > 10.0) reason += "Chain+ ";
        if (result.next_score > 5.0) reason += "Next+ ";
        if (height >= FIELD_HEIGHT - 2) reason += "Danger ";
        
        return reason;
    }
    
    // ヘルパーメソッド群
    int get_column_height(const Field& field, int x) const {
        for (int y = FIELD_HEIGHT - 1; y >= 0; --y) {
            if (field.get_puyo(Position(x, y)) != PuyoColor::EMPTY) {
                return FIELD_HEIGHT - y;
            }
        }
        return 0;
    }
    
    std::vector<int> get_all_column_heights(const Field& field) const {
        std::vector<int> heights;
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            heights.push_back(get_column_height(field, x));
        }
        return heights;
    }
    
    int count_same_color_adjacency(const Field& field, int x) const {
        int count = 0;
        int y = get_column_height(field, x);
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};
        
        // 現在位置の色（仮想的に配置するぷよの色は考慮せず、隣接数をカウント）
        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            if (nx >= 0 && nx < FIELD_WIDTH && ny >= 0 && ny < FIELD_HEIGHT) {
                if (field.get_puyo(Position(nx, ny)) != PuyoColor::EMPTY) {
                    count++;
                }
            }
        }
        
        return count;
    }
    
    // 基本位置評価（ChainSearchAI専用）
    double evaluate_basic_position(const Field& field, int x, int r) const {
        double score = 0.0;
        
        // 中央寄りボーナス
        int center_distance = std::abs(x - FIELD_WIDTH / 2);
        score += (3 - center_distance) * weights_.center_preference;
        
        // 低い位置ボーナス
        int height = get_column_height(field, x);
        score += (FIELD_HEIGHT - height) * weights_.height_balance;
        
        return score;
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