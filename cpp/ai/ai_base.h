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
    
    // NEXT情報（将来対応）
    // std::vector<PuyoPair> next_queue;
    
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
};

// AI作成用ファクトリ関数の型定義
using AIFactory = std::function<std::unique_ptr<AIBase>(const AIParameters&)>;

} // namespace ai
} // namespace puyo