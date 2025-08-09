#pragma once

#include "player.h"
#include <vector>
#include <memory>
#include <functional>

namespace puyo {

// ゲームモード
enum class GameMode {
    TOKOTON,    // とことんモード（1人プレイ）
    VERSUS      // 対戦モード（2人プレイ）
};

// ゲーム状態
enum class GameState {
    WAITING,        // 待機中
    PLAYING,        // プレイ中
    PAUSED,         // ポーズ中
    FINISHED        // 終了
};

// ゲームステップ
enum class GameStep {
    PUYO_SPAWN,     // ぷよ出現
    PLAYER_INPUT,   // プレイヤー入力待ち
    PUYO_PLACE,     // ぷよ設置
    GRAVITY,        // 落下処理
    CHAIN_PROCESS,  // 連鎖処理
    GARBAGE_DROP,   // おじゃまぷよ降下
    WIN_CHECK,      // 勝敗判定
    NEXT_TURN       // 次ターン準備
};

// ゲーム結果
struct GameResult {
    GameMode mode;
    int winner_id;              // 勝者ID（-1なら引き分け、とことんモードでは無効）
    std::vector<PlayerStats> player_stats;
    int total_turns;
    
    GameResult() : mode(GameMode::TOKOTON), winner_id(-1), total_turns(0) {}
};

// 入力インターフェース（将来のAI対応用）
using InputCallback = std::function<MoveCommand(int player_id)>;

class GameManager {
private:
    GameMode mode_;
    GameState state_;
    GameStep current_step_;
    std::vector<std::unique_ptr<Player>> players_;
    int current_player_;
    int turn_count_;
    bool time_limit_enabled_;
    int time_limit_ms_;
    
    // 入力コールバック
    InputCallback input_callback_;
    
public:
    GameManager(GameMode mode);
    ~GameManager() = default;
    
    // プレイヤー管理
    void add_player(const std::string& name, PlayerType type);
    Player* get_player(int player_id);
    const Player* get_player(int player_id) const;
    
    // ゲーム制御
    void start_game();
    void pause_game();
    void resume_game();
    void reset_game();
    GameResult finish_game();
    
    // ステップ実行
    bool execute_step();
    void execute_full_turn();
    
    // 入力設定
    void set_input_callback(const InputCallback& callback) { input_callback_ = callback; }
    
    // 状態取得
    GameMode get_mode() const { return mode_; }
    GameState get_state() const { return state_; }
    GameStep get_current_step() const { return current_step_; }
    int get_current_player() const { return current_player_; }
    int get_turn_count() const { return turn_count_; }
    
    // 時間制限設定
    void enable_time_limit(int milliseconds) { 
        time_limit_enabled_ = true; 
        time_limit_ms_ = milliseconds; 
    }
    void disable_time_limit() { time_limit_enabled_ = false; }
    
    // ゲーム状態確認
    bool is_game_finished() const;
    int get_winner() const;
    
    // デバッグ用
    std::string get_game_status() const;
    
private:
    // ステップ実行関数
    bool step_puyo_spawn();
    bool step_player_input();
    bool step_puyo_place();
    bool step_gravity();
    bool step_chain_process();
    bool step_garbage_drop();
    bool step_win_check();
    bool step_next_turn();
    
    // ユーティリティ
    void switch_to_next_player();
    void check_game_over();
};

} // namespace puyo