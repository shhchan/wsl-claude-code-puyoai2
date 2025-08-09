#include "game_manager.h"
#include <sstream>
#include <algorithm>

namespace puyo {

GameManager::GameManager(GameMode mode) 
    : mode_(mode), state_(GameState::WAITING), current_step_(GameStep::PUYO_SPAWN),
      current_player_(0), turn_count_(0), time_limit_enabled_(false), time_limit_ms_(0) {}

void GameManager::add_player(const std::string& name, PlayerType type) {
    int player_id = static_cast<int>(players_.size());
    players_.emplace_back(std::make_unique<Player>(player_id, name, type));
}

Player* GameManager::get_player(int player_id) {
    if (player_id >= 0 && player_id < static_cast<int>(players_.size())) {
        return players_[player_id].get();
    }
    return nullptr;
}

const Player* GameManager::get_player(int player_id) const {
    if (player_id >= 0 && player_id < static_cast<int>(players_.size())) {
        return players_[player_id].get();
    }
    return nullptr;
}

void GameManager::start_game() {
    if (players_.empty()) return;
    
    // プレイヤーの初期化
    for (auto& player : players_) {
        player->initialize_game();
    }
    
    current_player_ = 0;
    turn_count_ = 0;
    current_step_ = GameStep::PUYO_SPAWN;
    state_ = GameState::PLAYING;
}

void GameManager::pause_game() {
    if (state_ == GameState::PLAYING) {
        state_ = GameState::PAUSED;
    }
}

void GameManager::resume_game() {
    if (state_ == GameState::PAUSED) {
        state_ = GameState::PLAYING;
    }
}

void GameManager::reset_game() {
    for (auto& player : players_) {
        player->reset_game();
    }
    
    current_player_ = 0;
    turn_count_ = 0;
    current_step_ = GameStep::PUYO_SPAWN;
    state_ = GameState::WAITING;
}

GameResult GameManager::finish_game() {
    GameResult result;
    result.mode = mode_;
    result.total_turns = turn_count_;
    result.winner_id = get_winner();
    
    for (const auto& player : players_) {
        result.player_stats.push_back(player->get_stats());
    }
    
    state_ = GameState::FINISHED;
    return result;
}

bool GameManager::execute_step() {
    if (state_ != GameState::PLAYING) return false;
    
    bool step_completed = false;
    
    switch (current_step_) {
        case GameStep::PUYO_SPAWN:
            step_completed = step_puyo_spawn();
            break;
        case GameStep::PLAYER_INPUT:
            step_completed = step_player_input();
            break;
        case GameStep::PUYO_PLACE:
            step_completed = step_puyo_place();
            break;
        case GameStep::GRAVITY:
            step_completed = step_gravity();
            break;
        case GameStep::CHAIN_PROCESS:
            step_completed = step_chain_process();
            break;
        case GameStep::GARBAGE_DROP:
            step_completed = step_garbage_drop();
            break;
        case GameStep::WIN_CHECK:
            step_completed = step_win_check();
            break;
        case GameStep::NEXT_TURN:
            step_completed = step_next_turn();
            break;
    }
    
    return step_completed;
}

void GameManager::execute_full_turn() {
    while (state_ == GameState::PLAYING && !is_game_finished()) {
        if (!execute_step()) {
            break;  // ステップが完了しない場合（入力待ちなど）
        }
    }
}

bool GameManager::is_game_finished() const {
    if (mode_ == GameMode::TOKOTON) {
        // とことんモードは1人プレイヤーが敗北したら終了
        return players_[0]->is_game_over();
    } else {
        // 対戦モードは誰かが敗北したら終了
        for (const auto& player : players_) {
            if (player->is_game_over()) {
                return true;
            }
        }
    }
    return false;
}

int GameManager::get_winner() const {
    if (mode_ == GameMode::TOKOTON) {
        return -1;  // とことんモードには勝者なし
    }
    
    // 対戦モードでは、敗北していないプレイヤーが勝者
    for (const auto& player : players_) {
        if (!player->is_game_over()) {
            return player->get_id();
        }
    }
    
    return -1;  // 引き分け（全員敗北）
}

std::string GameManager::get_game_status() const {
    std::ostringstream oss;
    
    oss << "Game Mode: " << (mode_ == GameMode::TOKOTON ? "Tokoton" : "Versus") << "\n";
    oss << "State: ";
    
    switch (state_) {
        case GameState::WAITING:   oss << "Waiting"; break;
        case GameState::PLAYING:   oss << "Playing"; break;
        case GameState::PAUSED:    oss << "Paused"; break;
        case GameState::FINISHED:  oss << "Finished"; break;
    }
    
    oss << "\nCurrent Step: ";
    switch (current_step_) {
        case GameStep::PUYO_SPAWN:    oss << "Puyo Spawn"; break;
        case GameStep::PLAYER_INPUT:  oss << "Player Input"; break;
        case GameStep::PUYO_PLACE:    oss << "Puyo Place"; break;
        case GameStep::GRAVITY:       oss << "Gravity"; break;
        case GameStep::CHAIN_PROCESS: oss << "Chain Process"; break;
        case GameStep::GARBAGE_DROP:  oss << "Garbage Drop"; break;
        case GameStep::WIN_CHECK:     oss << "Win Check"; break;
        case GameStep::NEXT_TURN:     oss << "Next Turn"; break;
    }
    
    oss << "\nTurn: " << turn_count_ << "\n";
    oss << "Current Player: " << current_player_ << "\n";
    
    if (is_game_finished()) {
        int winner = get_winner();
        if (winner >= 0) {
            oss << "Winner: Player " << winner << "\n";
        } else {
            oss << "Game Over (No winner)\n";
        }
    }
    
    return oss.str();
}

// プライベートメソッドの実装

bool GameManager::step_puyo_spawn() {
    Player* player = get_player(current_player_);
    if (!player || player->is_game_over()) {
        current_step_ = GameStep::WIN_CHECK;
        return true;
    }
    
    // 新しいぷよペアを設定
    PuyoPair new_pair = player->get_next_generator().get_current_pair();
    player->get_controller().set_current_pair(new_pair);
    
    current_step_ = GameStep::PLAYER_INPUT;
    return true;
}

bool GameManager::step_player_input() {
    Player* player = get_player(current_player_);
    if (!player) return false;
    
    // 入力コールバックがある場合は実行
    if (input_callback_) {
        MoveCommand command = input_callback_(current_player_);
        if (command != MoveCommand::NONE) {
            bool move_result = player->get_controller().execute_command(command);
            if (!move_result || command == MoveCommand::DROP) {
                // 移動失敗または落下コマンドの場合、設置へ
                current_step_ = GameStep::PUYO_PLACE;
                return true;
            }
        }
    }
    
    // 入力待ちの場合はfalseを返す
    return false;
}

bool GameManager::step_puyo_place() {
    Player* player = get_player(current_player_);
    if (!player) return false;
    
    // ぷよを設置
    bool placed = player->get_controller().place_current_pair();
    if (!placed) {
        // 設置失敗 = ゲームオーバー
        player->set_state(PlayerState::DEFEATED);
        current_step_ = GameStep::WIN_CHECK;
        return true;
    }
    
    current_step_ = GameStep::GRAVITY;
    return true;
}

bool GameManager::step_gravity() {
    Player* player = get_player(current_player_);
    if (!player) return false;
    
    // 重力適用
    player->get_field().apply_gravity();
    
    current_step_ = GameStep::CHAIN_PROCESS;
    return true;
}

bool GameManager::step_chain_process() {
    Player* player = get_player(current_player_);
    if (!player) return false;
    
    // 連鎖実行
    ChainSystemResult chain_result = player->get_chain_system().execute_chains();
    
    if (chain_result.has_chains()) {
        // おじゃまぷよ計算
        GarbageResult garbage_result;
        int garbage_to_send = player->get_garbage_system().calculate_garbage_to_send(
            chain_result.score_result.chain_score);
        
        if (garbage_to_send > 0) {
            garbage_result.sent_garbage = garbage_to_send;
            
            // 対戦モードの場合、相手におじゃまぷよを送信
            if (mode_ == GameMode::VERSUS && players_.size() > 1) {
                int opponent_id = (current_player_ == 0) ? 1 : 0;
                Player* opponent = get_player(opponent_id);
                if (opponent && !opponent->is_game_over()) {
                    opponent->get_garbage_system().add_pending_garbage(garbage_to_send, current_player_);
                }
            }
        }
        
        // 統計更新
        player->update_stats(chain_result, garbage_result);
    }
    
    current_step_ = GameStep::GARBAGE_DROP;
    return true;
}

bool GameManager::step_garbage_drop() {
    Player* player = get_player(current_player_);
    if (!player) return false;
    
    // 予告おじゃまぷよがある場合は降下
    if (player->get_garbage_system().has_pending_garbage()) {
        GarbageResult garbage_result = player->get_garbage_system().drop_pending_garbage();
        
        // 統計更新
        ChainSystemResult empty_chain;
        player->update_stats(empty_chain, garbage_result);
        
        // 重力適用
        player->get_field().apply_gravity();
    }
    
    current_step_ = GameStep::WIN_CHECK;
    return true;
}

bool GameManager::step_win_check() {
    check_game_over();
    
    if (is_game_finished()) {
        state_ = GameState::FINISHED;
        return false;
    }
    
    current_step_ = GameStep::NEXT_TURN;
    return true;
}

bool GameManager::step_next_turn() {
    Player* player = get_player(current_player_);
    if (player) {
        // NEXTを更新
        player->get_next_generator().advance_to_next();
    }
    
    // 次のプレイヤーに切り替え（対戦モードの場合）
    if (mode_ == GameMode::VERSUS) {
        switch_to_next_player();
    }
    
    turn_count_++;
    current_step_ = GameStep::PUYO_SPAWN;
    return true;
}

void GameManager::switch_to_next_player() {
    if (players_.size() > 1) {
        current_player_ = (current_player_ + 1) % static_cast<int>(players_.size());
    }
}

void GameManager::check_game_over() {
    for (auto& player : players_) {
        if (player->get_field().is_game_over()) {
            player->set_state(PlayerState::DEFEATED);
        }
    }
}

} // namespace puyo