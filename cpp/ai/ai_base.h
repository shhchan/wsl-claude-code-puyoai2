#pragma once

#include "core/puyo_types.h"
#include "core/puyo_controller.h"
#include "core/field.h"
#include "core/player.h"
#include <string>
#include <memory>
#include <map>
#include <functional>
#include <vector>
#include <set>

namespace puyo {
namespace ai {

// AI設定パラメータ
using AIParameters = std::map<std::string, std::string>;

// ゲーム状態情報（AI思考用）
struct GameState {
    // 自分のフィールド状態
    const Field* own_field;
    
    // 相手のフィールド状態（対戦時）
    const Field* opponent_field;
    
    // 現在操作中のぷよペア
    PuyoPair current_pair;
    
    // NEXT情報（ネクスト・ネクネク情報）
    std::vector<PuyoPair> next_queue;
    
    // フィールド分析情報
    struct FieldAnalysis {
        int chain_potential;      // 連鎖ポテンシャル
        double stability_score;   // 安定性スコア
        int max_height;          // 最高の列の高さ
        int height_variance;     // 高さの分散
        
        FieldAnalysis() : chain_potential(0), stability_score(0.0), 
                         max_height(0), height_variance(0) {}
    } field_analysis;
    
    // 配置履歴情報
    struct PlacementHistory {
        std::vector<std::pair<int, int>> recent_placements; // (x, r)のペア
        int consecutive_chains;  // 連続連鎖数
        int turns_since_chain;   // 最後の連鎖からの経過ターン
        
        PlacementHistory() : consecutive_chains(0), turns_since_chain(0) {}
    } placement_history;
    
    // その他の情報
    int player_id;
    int turn_count;
    bool is_versus_mode;
    
    GameState() : own_field(nullptr), opponent_field(nullptr), 
                  player_id(-1), turn_count(0), is_versus_mode(false) {}
};

// AI思考結果
struct AIDecision {
    int x;                                      // 軸ぷよの列（0-5）
    int r;                                      // 回転状態（0:UP, 1:RIGHT, 2:DOWN, 3:LEFT）
    std::vector<MoveCommand> move_commands;     // (x, r)へ到達するまでの操作コマンドリスト
    double confidence;                          // 行動の確信度（0.0-1.0）
    std::string reason;                         // 判断理由（デバッグ用）
    
    AIDecision(int x_pos = -1, int rotation = 0, 
               const std::vector<MoveCommand>& commands = {},
               double conf = 0.0, 
               const std::string& r = "")
        : x(x_pos), r(rotation), move_commands(commands), confidence(conf), reason(r) {}
};

// MoveCommand生成ヘルパークラス
class MoveCommandGenerator {
public:
    // (x, r)の位置へ到達するMoveCommandリストを生成
    static std::vector<MoveCommand> generate_move_commands(const Field& field, int target_x, int target_r);
    
private:
    // 基本アルゴリズム：12段制約なし
    static std::vector<MoveCommand> generate_basic_commands(int target_x, int target_r);
    
    // 高度なアルゴリズム：12段制約対応
    static std::vector<MoveCommand> generate_advanced_commands(const Field& field, int target_x, int target_r);
    
    // 到達可能な列の算出
    static std::set<int> calculate_reachable_columns(const Field& field);
    
    // 11段列の検出
    static std::vector<int> find_11_height_columns(const Field& field, const std::set<int>& reachable);
    
    // 回転コマンド生成
    static std::vector<MoveCommand> generate_rotation_commands(int current_rotation, int target_rotation);
};

// AI基底クラス
class AIBase {
protected:
    std::string name_;
    AIParameters parameters_;
    bool initialized_;
    
public:
    AIBase(const std::string& name) 
        : name_(name), initialized_(false) {}
    
    virtual ~AIBase() = default;
    
    // AI情報
    const std::string& get_name() const { return name_; }
    bool is_initialized() const { return initialized_; }
    
    // パラメータ管理
    void set_parameter(const std::string& key, const std::string& value) {
        parameters_[key] = value;
    }
    
    std::string get_parameter(const std::string& key, const std::string& default_value = "") const {
        auto it = parameters_.find(key);
        return (it != parameters_.end()) ? it->second : default_value;
    }
    
    const AIParameters& get_all_parameters() const { return parameters_; }
    
    // 初期化・終了処理
    virtual bool initialize() {
        initialized_ = true;
        return true;
    }
    
    virtual void shutdown() {
        initialized_ = false;
    }
    
    // 思考処理（純粋仮想関数）
    virtual AIDecision think(const GameState& state) = 0;
    
    // 思考時間制限（デフォルトは無制限）
    virtual int get_think_time_ms() const { return -1; }
    
    // デバッグ情報
    virtual std::string get_debug_info() const { return ""; }
    
    // AI種別情報（サブクラスで定義）
    virtual std::string get_type() const = 0;
    virtual std::string get_version() const { return "1.0"; }

protected:
    // ヘルパーメソッド：next情報を活用するための統一インターフェース
    
    // フィールド分析情報を計算
    GameState::FieldAnalysis calculate_field_analysis(const Field& field) const {
        GameState::FieldAnalysis analysis;
        
        // 各列の高さを計算
        std::vector<int> column_heights;
        int max_height = 0;
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            int height = get_column_height(field, x);
            column_heights.push_back(height);
            if (height > max_height) max_height = height;
        }
        analysis.max_height = max_height;
        
        // 高さの分散を計算
        double mean_height = 0.0;
        for (int h : column_heights) mean_height += h;
        mean_height /= FIELD_WIDTH;
        
        double variance = 0.0;
        for (int h : column_heights) {
            variance += (h - mean_height) * (h - mean_height);
        }
        analysis.height_variance = static_cast<int>(variance / FIELD_WIDTH);
        
        // 安定性スコアの計算（隣接列の高さの差が小さいほど高い）
        analysis.stability_score = calculate_stability_score(column_heights);
        
        // 連鎖ポテンシャル（簡易版）
        analysis.chain_potential = calculate_chain_potential(field);
        
        return analysis;
    }
    
    // 配置可能な全ての(x, r)組み合わせを取得
    std::vector<std::pair<int, int>> get_all_valid_positions(const Field& field) const {
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
    
    // ネクスト情報を考慮した評価
    double evaluate_with_next_info(const Field& field, int x, int r, 
                                  const std::vector<PuyoPair>& next_queue) const {
        // 基本評価（サブクラスでオーバーライド）
        double base_score = evaluate_position_basic(field, x, r);
        
        // ネクスト情報を考慮した追加評価
        if (!next_queue.empty()) {
            base_score += evaluate_next_compatibility(field, x, r, next_queue[0]);
            
            // ネクネクも考慮（利用可能な場合）
            if (next_queue.size() > 1) {
                base_score += evaluate_next_compatibility(field, x, r, next_queue[1]) * 0.5;
            }
        }
        
        return base_score;
    }

private:
    // 列の高さを取得
    int get_column_height(const Field& field, int x) const {
        for (int y = FIELD_HEIGHT - 1; y >= 0; --y) {
            if (field.get_puyo(Position(x, y)) != PuyoColor::EMPTY) {
                return FIELD_HEIGHT - y;
            }
        }
        return 0;
    }
    
    // 安定性スコアの計算
    double calculate_stability_score(const std::vector<int>& heights) const {
        double stability = 0.0;
        
        for (size_t i = 1; i < heights.size(); ++i) {
            int diff = std::abs(heights[i] - heights[i-1]);
            stability += std::max(0, 5 - diff); // 差が小さいほど高いスコア
        }
        
        return stability / (heights.size() - 1);
    }
    
    // 簡易連鎖ポテンシャル計算
    int calculate_chain_potential(const Field& field) const {
        int potential = 0;
        
        // 各色のぷよ数をカウント
        std::map<PuyoColor, int> color_counts;
        for (int y = 0; y < FIELD_HEIGHT; ++y) {
            for (int x = 0; x < FIELD_WIDTH; ++x) {
                PuyoColor color = field.get_puyo(Position(x, y));
                if (color != PuyoColor::EMPTY) {
                    color_counts[color]++;
                }
            }
        }
        
        // 各色について、4個に近いほど高いポテンシャル
        for (const auto& pair : color_counts) {
            int count = pair.second;
            if (count >= 3) {
                potential += (count / 4) * 10 + (count % 4) * 2;
            }
        }
        
        return potential;
    }
    
    // 基本位置評価（サブクラスでオーバーライド可能）
    virtual double evaluate_position_basic(const Field& field, int x, int r) const {
        // デフォルト実装：中央寄り + 低い位置を優先
        double score = 0.0;
        
        // 中央寄りボーナス
        int center_distance = std::abs(x - FIELD_WIDTH / 2);
        score += (3 - center_distance) * 2.0;
        
        // 低い位置ボーナス
        int height = get_column_height(field, x);
        score += (FIELD_HEIGHT - height) * 1.0;
        
        return score;
    }
    
    // ネクスト情報との互換性評価
    double evaluate_next_compatibility(const Field& field, int x, int r, 
                                     const PuyoPair& next_pair) const {
        double compatibility = 0.0;
        
        // 現在の配置と次のペアの色の関連性を評価
        PuyoColor axis_color = next_pair.axis;
        PuyoColor child_color = next_pair.child;
        
        // 周辺にある同色ぷよとの相性をチェック
        int nearby_axis_count = count_nearby_color(field, x, axis_color);
        int nearby_child_count = count_nearby_color(field, x, child_color);
        
        compatibility += nearby_axis_count * 3.0;
        compatibility += nearby_child_count * 3.0;
        
        return compatibility;
    }
    
    // 指定位置周辺の特定色ぷよ数をカウント
    int count_nearby_color(const Field& field, int x, PuyoColor target_color) const {
        int count = 0;
        int dx[] = {-1, 1, 0, 0, -1, -1, 1, 1};
        int dy[] = {0, 0, -1, 1, -1, 1, -1, 1};
        
        int y = get_column_height(field, x);
        
        for (int i = 0; i < 8; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            if (nx >= 0 && nx < FIELD_WIDTH && ny >= 0 && ny < FIELD_HEIGHT) {
                if (field.get_puyo(Position(nx, ny)) == target_color) {
                    count++;
                }
            }
        }
        
        return count;
    }
};

// AI作成用ファクトリ関数の型定義
using AIFactory = std::function<std::unique_ptr<AIBase>(const AIParameters&)>;

} // namespace ai
} // namespace puyo