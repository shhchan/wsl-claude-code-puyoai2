#pragma once

#include "field.h"
#include "next_generator.h"
#include "puyo_controller.h"
#include "chain_system.h"
#include "garbage_system.h"
#include <string>

namespace puyo {

// プレイヤーの種類
enum class PlayerType {
    HUMAN,    // 人間プレイヤー
    AI        // AIプレイヤー（将来の拡張用）
};

// プレイヤー状態
enum class PlayerState {
    ACTIVE,      // アクティブ（プレイ中）
    DEFEATED,    // 敗北
    PAUSED       // ポーズ中
};

// プレイヤー統計
struct PlayerStats {
    int total_score;        // 合計得点
    int total_chains;       // 合計連鎖数
    int max_chain;         // 最大連鎖数
    int sent_garbage;      // 送信したおじゃまぷよ数
    int received_garbage;  // 受信したおじゃまぷよ数
    
    PlayerStats() : total_score(0), total_chains(0), max_chain(0), 
                   sent_garbage(0), received_garbage(0) {}
};

class Player {
private:
    int player_id_;
    std::string name_;
    PlayerType type_;
    PlayerState state_;
    
    // ゲーム要素
    Field field_;
    NextGenerator next_generator_;
    PuyoController controller_;
    ChainSystem chain_system_;
    GarbageSystem garbage_system_;
    
    // 統計
    PlayerStats stats_;
    
public:
    Player(int player_id, const std::string& name, PlayerType type);
    
    // プレイヤー情報
    int get_id() const { return player_id_; }
    const std::string& get_name() const { return name_; }
    PlayerType get_type() const { return type_; }
    PlayerState get_state() const { return state_; }
    void set_state(PlayerState state) { state_ = state; }
    
    // ゲーム要素へのアクセス
    Field& get_field() { return field_; }
    const Field& get_field() const { return field_; }
    
    NextGenerator& get_next_generator() { return next_generator_; }
    const NextGenerator& get_next_generator() const { return next_generator_; }
    
    PuyoController& get_controller() { return controller_; }
    const PuyoController& get_controller() const { return controller_; }
    
    ChainSystem& get_chain_system() { return chain_system_; }
    const ChainSystem& get_chain_system() const { return chain_system_; }
    
    GarbageSystem& get_garbage_system() { return garbage_system_; }
    const GarbageSystem& get_garbage_system() const { return garbage_system_; }
    
    // 統計
    const PlayerStats& get_stats() const { return stats_; }
    void update_stats(const ChainSystemResult& chain_result, const GarbageResult& garbage_result);
    
    // ゲーム制御
    void initialize_game();
    void reset_game();
    bool is_game_over() const;
    
    // デバッグ用
    std::string get_status() const;
};

} // namespace puyo