#include "../cpp/ai/ai_base.h"
#include "../cpp/ai/random_ai.h"
#include "../cpp/core/field.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace puyo;
using namespace puyo::ai;

// コマンド表示用のデバッグ関数
std::string move_command_to_string(std::vector<MoveCommand> commands) {
    std::string result;
    for (const auto& cmd : commands) {
        switch (cmd) {
            case MoveCommand::LEFT: result += "LEFT "; break;
            case MoveCommand::RIGHT: result += "RIGHT "; break;
            case MoveCommand::ROTATE_CW: result += "ROTATE_CW "; break;
            case MoveCommand::ROTATE_CCW: result += "ROTATE_CCW "; break;
            case MoveCommand::DROP: result += "DROP "; break;
        }
    }
    return result;
}

void test_move_command_generator_basic() {
    std::cout << "Testing MoveCommandGenerator basic functionality..." << std::endl;
    
    Field field;  // 空のフィールド
    
    // 基本的な移動テスト：(2, 0) → (0, 0)
    auto commands = MoveCommandGenerator::generate_move_commands(field, 0, 0);
    
    // 2列目から0列目への移動なので、LEFT × 2 + DROP が期待される
    assert(!commands.empty());
    assert(commands.size() >= 3);
    assert(commands[0] == MoveCommand::LEFT);
    assert(commands[1] == MoveCommand::LEFT);

    // 最後はDROPコマンドであることを確認
    assert(commands.back() == MoveCommand::DROP);
    
    std::cout << "Generated " << commands.size() << " commands for (0, 0)" << std::endl;

    // commandsの内容を表示
    std::cout << "Commands: " << move_command_to_string(commands) << std::endl;
    
    std::cout << "MoveCommandGenerator basic test: OK" << std::endl;
}

void test_move_command_generator_rotation() {
    std::cout << "Testing MoveCommandGenerator rotation..." << std::endl;
    
    Field field;  // 空のフィールド
    
    // 回転テスト：(2, 2) - 2列目でDOWN回転
    auto commands = MoveCommandGenerator::generate_move_commands(field, 2, 2);
    
    assert(!commands.empty());
    assert(commands.back() == MoveCommand::DROP);
    
    // 回転コマンドが2回含まれていることを確認
    int rotation_count = 0;
    for (const auto& cmd : commands) {
        if (cmd == MoveCommand::ROTATE_CW || cmd == MoveCommand::ROTATE_CCW) {
            rotation_count++;
        }
    }
    assert(rotation_count == 2);  // DOWN回転は2回
    
    std::cout << "Generated " << commands.size() << " commands for (2, 2)" << std::endl;

    // commandsの内容を表示
    std::cout << "Commands: " << move_command_to_string(commands) << std::endl;
    
    std::cout << "MoveCommandGenerator rotation test: OK" << std::endl;
}

void test_ai_decision_structure() {
    std::cout << "Testing AIDecision structure..." << std::endl;
    
    // 新しいAIDecision構造体のテスト
    std::vector<MoveCommand> test_commands = {
        MoveCommand::LEFT,
        MoveCommand::ROTATE_CW,
        MoveCommand::ROTATE_CW,
        MoveCommand::DROP
    };
    
    AIDecision decision(1, 2, test_commands, 0.8, "Test decision");
    
    assert(decision.x == 1);
    assert(decision.r == 2);
    assert(decision.move_commands.size() == 4);
    assert(decision.confidence == 0.8);
    assert(decision.reason == "Test decision");
    
    // move_commandsの内容を表示
    std::cout << "AIDecision commands: " << move_command_to_string(decision.move_commands) << std::endl;

    std::cout << "AIDecision structure test: OK" << std::endl;
}

void test_random_ai_improved() {
    std::cout << "Testing improved RandomAI..." << std::endl;
    
    // RandomAIインスタンスを作成
    AIParameters params;
    params["seed"] = "12345";  // 再現可能なテスト用
    RandomAI ai(params);
    
    assert(ai.initialize());
    assert(ai.is_initialized());
    assert(ai.get_type() == "Random");
    
    // ゲーム状態を準備
    Field field;
    GameState state;
    state.own_field = &field;
    state.current_pair = PuyoPair(PuyoColor::RED, PuyoColor::BLUE, Position(2, 11), Rotation::UP);
    
    // AIに思考させる
    AIDecision decision = ai.think(state);
    
    // 新しいAIDecision形式かチェック
    assert(decision.x >= 0 && decision.x < FIELD_WIDTH);
    assert(decision.r >= 0 && decision.r < 4);
    assert(!decision.move_commands.empty());
    assert(decision.confidence > 0.0);
    assert(!decision.reason.empty());
    
    // MoveCommandリストの最後はDROPであることを確認
    assert(decision.move_commands.back() == MoveCommand::DROP);
    
    std::cout << "RandomAI decision: (" << decision.x << ", " << decision.r << ") with " 
              << decision.move_commands.size() << " commands" << std::endl;
    std::cout << "Reason: " << decision.reason << std::endl;

    // move_commandsの内容を表示
    std::cout << "Commands: " << move_command_to_string(decision.move_commands) << std::endl;
    
    std::cout << "Improved RandomAI test: OK" << std::endl;
}

void test_field_with_12_height() {
    std::cout << "Testing 12-height field constraint..." << std::endl;
    
    Field field;
    
    // 2列目に12段の高さまでぷよを積む
    for (int row = 0; row < 12; ++row) {
        field.set_puyo(Position(1, row), PuyoColor::RED);
    }
    // 5列目に11段の高さまでぷよを積む
    for (int row = 0; row < 11; ++row) {
        field.set_puyo(Position(4, row), PuyoColor::BLUE);
    }
    
    // MoveCommandGeneratorが高度なアルゴリズムを使用することをテスト
    auto commands = MoveCommandGenerator::generate_move_commands(field, 0, 0);
    
    assert(!commands.empty());
    assert(commands.back() == MoveCommand::DROP);

    std::cout << "Generated " << commands.size() << " commands for constrained field" << std::endl;
    std::cout << "Commands: " << move_command_to_string(commands) << std::endl;

    // コマンドが生成されることを確認（実際の実装では11コマンド）
    // RIGHT x 2, ROTATE_CCW x 2, ROTATE_CCW, LEFT x 4, ROTATE_CCW, DROP
    assert(commands.size() == 11);
    
    std::cout << "12-height constraint test: OK" << std::endl;
}

void test_can_place_integration() {
    std::cout << "Testing can_place integration..." << std::endl;
    
    Field field;
    
    // フィールド上部をほぼ埋める（配置可能位置を制限）
    for (int col = 0; col < FIELD_WIDTH; ++col) {
        for (int row = 0; row < 14; ++row) {
            if (col != 2) {  // 2列目だけ空けておく
                field.set_puyo(Position(col, row), PuyoColor::RED);
            }
        }
    }
    // 14段目フラグを2列目以外設定
    for (int col = 0; col < FIELD_WIDTH; ++col) {
        if (col != 2) {
            field.mark_row14_used(col);
        }
    }
    
    AIParameters params;
    params["seed"] = "54321";
    RandomAI ai(params);
    ai.initialize();
    
    GameState state;
    state.own_field = &field;
    state.current_pair = PuyoPair(PuyoColor::GREEN, PuyoColor::YELLOW, Position(2, 11), Rotation::UP);
    
    AIDecision decision = ai.think(state);
    
    // 配置可能な位置が限定されているので、適切な位置（x=2）が選ばれることを確認
    assert(decision.x == 2);
    assert(!decision.move_commands.empty());
    
    std::cout << "Constrained field decision: (" << decision.x << ", " << decision.r << ")" << std::endl;
    std::cout << "Commands: " << move_command_to_string(decision.move_commands) << std::endl;
    
    std::cout << "can_place integration test: OK" << std::endl;
}

int main() {
    std::cout << "=== AI Decision Improvements (Ticket 018) Tests ===" << std::endl;
    
    try {
        test_ai_decision_structure();
        test_move_command_generator_basic();
        test_move_command_generator_rotation();
        test_random_ai_improved();
        test_field_with_12_height();
        test_can_place_integration();
        
        std::cout << "\n=== All tests passed! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}