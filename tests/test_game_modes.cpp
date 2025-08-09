#include "../cpp/core/game_manager.h"
#include <iostream>
#include <cassert>

using namespace puyo;

void test_player_creation() {
    std::cout << "Testing player creation and initialization..." << std::endl;
    
    Player player(0, "TestPlayer", PlayerType::HUMAN);
    
    assert(player.get_id() == 0);
    assert(player.get_name() == "TestPlayer");
    assert(player.get_type() == PlayerType::HUMAN);
    assert(player.get_state() == PlayerState::ACTIVE);
    assert(!player.is_game_over());
    
    // フィールドが初期化されていることを確認
    bool field_empty = true;
    for (int x = 0; x < FIELD_WIDTH; ++x) {
        for (int y = 0; y < FIELD_HEIGHT; ++y) {
            if (player.get_field().get_puyo(Position(x, y)) != PuyoColor::EMPTY) {
                field_empty = false;
                break;
            }
        }
    }
    assert(field_empty);
    
    std::cout << "Player creation: OK" << std::endl;
}

void test_tokoton_mode_setup() {
    std::cout << "Testing tokoton mode setup..." << std::endl;
    
    GameManager game(GameMode::TOKOTON);
    game.add_player("Player1", PlayerType::HUMAN);
    
    assert(game.get_mode() == GameMode::TOKOTON);
    assert(game.get_state() == GameState::WAITING);
    
    Player* player1 = game.get_player(0);
    assert(player1 != nullptr);
    assert(player1->get_name() == "Player1");
    
    std::cout << "Tokoton mode setup: OK" << std::endl;
}

void test_versus_mode_setup() {
    std::cout << "Testing versus mode setup..." << std::endl;
    
    GameManager game(GameMode::VERSUS);
    game.add_player("Player1", PlayerType::HUMAN);
    game.add_player("Player2", PlayerType::HUMAN);
    
    assert(game.get_mode() == GameMode::VERSUS);
    assert(game.get_state() == GameState::WAITING);
    
    Player* player1 = game.get_player(0);
    Player* player2 = game.get_player(1);
    assert(player1 != nullptr);
    assert(player2 != nullptr);
    assert(player1->get_name() == "Player1");
    assert(player2->get_name() == "Player2");
    
    std::cout << "Versus mode setup: OK" << std::endl;
}

void test_game_step_progression() {
    std::cout << "Testing game step progression..." << std::endl;
    
    GameManager game(GameMode::TOKOTON);
    game.add_player("Player1", PlayerType::HUMAN);
    
    // 入力コールバックを設定（自動的にDROPコマンドを送信）
    game.set_input_callback([](int player_id) -> MoveCommand {
        return MoveCommand::DROP;
    });
    
    game.start_game();
    assert(game.get_state() == GameState::PLAYING);
    assert(game.get_current_step() == GameStep::PUYO_SPAWN);
    
    // いくつかのステップを実行
    bool step1 = game.execute_step();  // PUYO_SPAWN -> PLAYER_INPUT
    assert(step1);
    assert(game.get_current_step() == GameStep::PLAYER_INPUT);
    
    bool step2 = game.execute_step();  // PLAYER_INPUT -> PUYO_PLACE (DROPコマンド)
    assert(step2);
    assert(game.get_current_step() == GameStep::PUYO_PLACE);
    
    bool step3 = game.execute_step();  // PUYO_PLACE -> GRAVITY
    assert(step3);
    assert(game.get_current_step() == GameStep::GRAVITY);
    
    std::cout << "Game step progression: OK" << std::endl;
}

void test_garbage_interaction_between_players() {
    std::cout << "Testing garbage interaction between players..." << std::endl;
    
    GameManager game(GameMode::VERSUS);
    game.add_player("Player1", PlayerType::HUMAN);
    game.add_player("Player2", PlayerType::HUMAN);
    
    game.start_game();
    
    Player* player1 = game.get_player(0);
    Player* player2 = game.get_player(1);
    
    // Player1のフィールドに連鎖を設定
    Field& field1 = player1->get_field();
    field1.set_puyo(Position(0, 0), PuyoColor::RED);
    field1.set_puyo(Position(0, 1), PuyoColor::RED);
    field1.set_puyo(Position(0, 2), PuyoColor::RED);
    field1.set_puyo(Position(0, 3), PuyoColor::RED);
    field1.set_puyo(Position(1, 0), PuyoColor::BLUE);
    field1.set_puyo(Position(1, 1), PuyoColor::BLUE);
    field1.set_puyo(Position(0, 4), PuyoColor::BLUE);
    field1.set_puyo(Position(0, 5), PuyoColor::BLUE);
    
    // 連鎖を実行してスコアを計算
    ChainSystemResult chain_result = player1->get_chain_system().execute_chains();
    assert(chain_result.has_chains());
    
    // おじゃまぷよを計算・送信
    int garbage_to_send = player1->get_garbage_system().calculate_garbage_to_send(
        chain_result.score_result.chain_score);
    
    if (garbage_to_send > 0) {
        player2->get_garbage_system().add_pending_garbage(garbage_to_send, 0);
        
        // Player2におじゃまぷよが予告されていることを確認
        assert(player2->get_garbage_system().has_pending_garbage());
        assert(player2->get_garbage_system().get_pending_garbage_count() == garbage_to_send);
    }
    
    std::cout << "Garbage interaction between players: OK" << std::endl;
}

void test_game_over_detection() {
    std::cout << "Testing game over detection..." << std::endl;
    
    GameManager game(GameMode::TOKOTON);
    game.add_player("Player1", PlayerType::HUMAN);
    
    game.start_game();
    Player* player = game.get_player(0);
    
    // 窒息点にぷよを配置してゲームオーバーをトリガー
    player->get_field().set_puyo(Position(2, 11), PuyoColor::RED);  // 窒息点
    
    assert(player->is_game_over());
    assert(game.is_game_finished());
    
    GameResult result = game.finish_game();
    assert(result.mode == GameMode::TOKOTON);
    assert(game.get_state() == GameState::FINISHED);
    
    std::cout << "Game over detection: OK" << std::endl;
}

void test_versus_mode_winner_detection() {
    std::cout << "Testing versus mode winner detection..." << std::endl;
    
    GameManager game(GameMode::VERSUS);
    game.add_player("Player1", PlayerType::HUMAN);
    game.add_player("Player2", PlayerType::HUMAN);
    
    game.start_game();
    
    Player* player1 = game.get_player(0);
    Player* player2 = game.get_player(1);
    
    // Player2を敗北させる
    player2->get_field().set_puyo(Position(2, 11), PuyoColor::RED);
    player2->set_state(PlayerState::DEFEATED);
    
    assert(game.is_game_finished());
    assert(game.get_winner() == 0);  // Player1が勝者
    
    GameResult result = game.finish_game();
    assert(result.winner_id == 0);
    assert(result.mode == GameMode::VERSUS);
    
    std::cout << "Versus mode winner detection: OK" << std::endl;
}

void test_pause_resume_functionality() {
    std::cout << "Testing pause/resume functionality..." << std::endl;
    
    GameManager game(GameMode::TOKOTON);
    game.add_player("Player1", PlayerType::HUMAN);
    
    game.start_game();
    assert(game.get_state() == GameState::PLAYING);
    
    game.pause_game();
    assert(game.get_state() == GameState::PAUSED);
    
    game.resume_game();
    assert(game.get_state() == GameState::PLAYING);
    
    std::cout << "Pause/resume functionality: OK" << std::endl;
}

int main() {
    std::cout << "=== Game Modes Tests ===" << std::endl;
    
    try {
        test_player_creation();
        test_tokoton_mode_setup();
        test_versus_mode_setup();
        test_game_step_progression();
        test_garbage_interaction_between_players();
        test_game_over_detection();
        test_versus_mode_winner_detection();
        test_pause_resume_functionality();
        
        std::cout << "\n✅ All game modes tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}