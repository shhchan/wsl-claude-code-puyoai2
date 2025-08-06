#include "../cpp/core/puyo_types.h"
#include "../cpp/core/field.h"
#include "../cpp/core/puyo_controller.h"
#include "../cpp/core/next_generator.h"
#include <iostream>
#include <cassert>

using namespace puyo;

void test_field_basic() {
    std::cout << "Testing Field basic operations..." << std::endl;
    
    Field field;
    
    // 空フィールドのテスト
    assert(field.get_puyo(Position(0, 0)) == PuyoColor::EMPTY);
    assert(!field.is_game_over());
    
    // ぷよ配置テスト
    field.set_puyo(Position(0, 0), PuyoColor::RED);
    assert(field.get_puyo(Position(0, 0)) == PuyoColor::RED);
    
    // 窒息点テスト
    field.set_puyo(Position(2, 11), PuyoColor::BLUE);  // 窒息点に配置
    assert(field.is_game_over());
    
    std::cout << "Field basic operations: OK" << std::endl;
}

void test_puyo_pair_rotation() {
    std::cout << "Testing PuyoPair rotation..." << std::endl;
    
    PuyoPair pair(PuyoColor::RED, PuyoColor::BLUE, Position(2, 5), Rotation::UP);
    
    // 初期状態
    Position child_pos = pair.get_child_position();
    assert(child_pos.x == 2 && child_pos.y == 6);  // 軸ぷよの上
    
    // 右回転
    pair.rot = Rotation::RIGHT;
    child_pos = pair.get_child_position();
    assert(child_pos.x == 3 && child_pos.y == 5);  // 軸ぷよの右
    
    std::cout << "PuyoPair rotation: OK" << std::endl;
}

void test_14th_row_special() {
    std::cout << "Testing 14th row special rules..." << std::endl;
    
    Field field;
    
    // 14段目への初回配置
    assert(field.can_place_at_row14(0));
    field.mark_row14_used(0);
    assert(!field.can_place_at_row14(0));
    assert(field.is_row14_used(0));
    
    // 14段目配置テスト（子ぷよが14段目を超えない配置）
    PuyoPair pair14(PuyoColor::RED, PuyoColor::BLUE, Position(1, 12), Rotation::UP);
    assert(field.can_place_puyo_pair(pair14));
    
    field.place_puyo_pair(pair14);
    assert(field.is_row14_used(1));

    // 回転方向が下向きの場合、14段目への配置は不可
    PuyoPair pair14_down(PuyoColor::GREEN, PuyoColor::YELLOW, Position(0, 13), Rotation::DOWN);
    assert(!field.can_place_puyo_pair(pair14_down));
    
    // 同じ列への2回目配置は不可
    // フィールドを一回クリアする
    field.clear();
    field.mark_row14_used(1);  // 1列目は使用済み
    PuyoPair pair14_2(PuyoColor::GREEN, PuyoColor::YELLOW, Position(1, 13), Rotation::UP);
    assert(!field.can_place_puyo_pair(pair14_2));
    
    std::cout << "14th row special rules: OK" << std::endl;
}

void test_gravity() {
    std::cout << "Testing gravity system..." << std::endl;
    
    Field field;
    
    // 浮いているぷよを配置
    field.set_puyo(Position(0, 6), PuyoColor::RED);
    field.set_puyo(Position(0, 4), PuyoColor::BLUE);
    field.set_puyo(Position(0, 3), PuyoColor::RED);
    field.set_puyo(Position(1, 0), PuyoColor::YELLOW);
    
    // 落下処理前の状態確認
    assert(field.get_puyo(Position(0, 0)) == PuyoColor::EMPTY);
    assert(field.get_puyo(Position(0, 1)) == PuyoColor::EMPTY);
    assert(field.get_puyo(Position(0, 2)) == PuyoColor::EMPTY);
    assert(field.get_puyo(Position(1, 0)) == PuyoColor::YELLOW);
    
    // 落下処理実行
    bool moved = field.apply_gravity();
    assert(moved);
    
    // 落下後の状態確認
    assert(field.get_puyo(Position(0, 0)) == PuyoColor::RED);
    assert(field.get_puyo(Position(0, 1)) == PuyoColor::BLUE);
    assert(field.get_puyo(Position(0, 2)) == PuyoColor::RED);
    assert(field.get_puyo(Position(0, 3)) == PuyoColor::EMPTY);
    assert(field.get_puyo(Position(0, 4)) == PuyoColor::EMPTY);
    assert(field.get_puyo(Position(0, 6)) == PuyoColor::EMPTY);
    assert(field.get_puyo(Position(1, 0)) == PuyoColor::YELLOW);
    
    std::cout << "Gravity system: OK" << std::endl;
}

void test_next_generator() {
    std::cout << "Testing NEXT generator..." << std::endl;
    
    NextGenerator gen(12345);  // 固定シード
    gen.initialize_next_sequence();
    
    // NEXT取得テスト
    PuyoPair current = gen.get_current_pair();
    PuyoPair next = gen.get_next_pair(1);
    
    assert(current.axis != PuyoColor::EMPTY);
    assert(current.child != PuyoColor::EMPTY);
    assert(next.axis != PuyoColor::EMPTY);
    assert(next.child != PuyoColor::EMPTY);
    
    // 順送りテスト
    PuyoPair old_next = gen.get_next_pair(1);
    gen.advance_to_next();
    PuyoPair new_current = gen.get_current_pair();
    
    assert(old_next.axis == new_current.axis);
    assert(old_next.child == new_current.child);
    
    std::cout << "NEXT generator: OK" << std::endl;
}

void test_puyo_controller() {
    std::cout << "Testing PuyoController..." << std::endl;
    
    Field field;
    PuyoController controller(&field);
    
    PuyoPair test_pair(PuyoColor::RED, PuyoColor::BLUE, Position(1, 5), Rotation::UP);
    controller.set_current_pair(test_pair);
    
    // 移動テスト
    assert(controller.can_move_left());
    assert(controller.can_move_right());
    assert(controller.can_move_down());
    
    controller.move_right();
    assert(controller.get_current_pair().pos.x == 2);
    
    controller.move_left();
    controller.move_left();
    assert(controller.get_current_pair().pos.x == 0);
    
    // 回転テスト
    assert(controller.can_rotate_clockwise());
    controller.rotate_clockwise();
    assert(controller.get_current_pair().rot == Rotation::RIGHT);

    // キックテスト（壁キック）
    controller.rotate_counter_clockwise();  // 回転方向を上に戻す
    controller.rotate_counter_clockwise();  // 左が壁なのでキックするはず
    assert(controller.get_current_pair().pos.x == 1);

    // キックテスト（床キック）
    controller.set_current_pair(PuyoPair(PuyoColor::RED, PuyoColor::BLUE, Position(2, 0), Rotation::RIGHT));
    controller.rotate_clockwise();  // 床に接触しているのでキックされるはず
    assert(controller.get_current_pair().pos.y == 1);
    
    std::cout << "PuyoController: OK" << std::endl;
}

int main() {
    std::cout << "=== Game Engine Core Tests ===" << std::endl;
    
    try {
        test_field_basic();
        test_puyo_pair_rotation();
        test_14th_row_special();
        test_gravity();
        test_next_generator();
        test_puyo_controller();
        
        std::cout << "\n✅ All tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}