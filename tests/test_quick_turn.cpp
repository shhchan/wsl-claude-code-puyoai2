#include "../cpp/core/puyo_types.h"
#include "../cpp/core/field.h"
#include "../cpp/core/puyo_controller.h"
#include <iostream>
#include <cassert>

using namespace puyo;

void test_first_rotation_no_rotate_when_blocked() {
    std::cout << "Testing first rotation does not rotate when both sides blocked..." << std::endl;
    
    Field field;
    PuyoController controller(&field);
    
    // 両側を塞ぐようにぷよを配置
    field.set_puyo(Position(2, 1), PuyoColor::RED);   // 左側
    field.set_puyo(Position(4, 1), PuyoColor::BLUE);  // 右側
    
    // 操作ぷよを中央に配置
    PuyoPair pair(PuyoColor::GREEN, PuyoColor::YELLOW, Position(3, 1), Rotation::UP);
    controller.set_current_pair(pair);
    
    Rotation original_rot = controller.get_current_pair().rot;
    
    // 1回目の回転操作 - 回転しないはず
    bool result = controller.rotate_clockwise();
    assert(!result);  // 回転は成功しない（見た目上）
    
    Rotation after_first_rot = controller.get_current_pair().rot;
    assert(original_rot == after_first_rot);  // 回転状態は変わらない
    
    std::cout << "First rotation no rotate when blocked: OK" << std::endl;
}

void test_second_rotation_performs_quick_turn() {
    std::cout << "Testing second rotation performs quick turn..." << std::endl;
    
    Field field;
    PuyoController controller(&field);
    
    // 両側を塞ぐようにぷよを配置
    field.set_puyo(Position(2, 1), PuyoColor::RED);   // 左側
    field.set_puyo(Position(4, 1), PuyoColor::BLUE);  // 右側
    
    // 操作ぷよを中央に配置
    PuyoPair pair(PuyoColor::GREEN, PuyoColor::YELLOW, Position(3, 1), Rotation::UP);
    controller.set_current_pair(pair);
    
    // 1回目の回転操作（フラグ設定）
    controller.rotate_clockwise();
    
    // 2回目の回転操作 - 180度回転するはず
    bool result = controller.rotate_clockwise();
    assert(result);  // 回転成功
    
    Rotation after_second_rot = controller.get_current_pair().rot;
    assert(Rotation::DOWN == after_second_rot);  // 180度回転（UP → DOWN）
    
    std::cout << "Second rotation performs quick turn: OK" << std::endl;
}

void test_flag_reset_on_move() {
    std::cout << "Testing flag reset on move..." << std::endl;
    
    Field field;
    PuyoController controller(&field);
    
    // 両側を塞ぐ
    field.set_puyo(Position(2, 1), PuyoColor::RED);
    field.set_puyo(Position(4, 1), PuyoColor::BLUE);
    
    PuyoPair pair(PuyoColor::GREEN, PuyoColor::YELLOW, Position(3, 1), Rotation::UP);
    controller.set_current_pair(pair);
    
    // 1回目の回転操作でフラグ設定
    controller.rotate_clockwise();
    
    // 右のぷよを削除
    field.set_puyo(Position(4, 1), PuyoColor::EMPTY);

    //右移動操作でフラグリセット
    controller.move_right();
    
    // 再度回転操作 - 向きは RIGHT になるはず
    bool result = controller.rotate_clockwise();
    assert(Rotation::RIGHT == controller.get_current_pair().rot);  // フラグがリセットされているので DOWN（180回転）ではなく RIGHT（通常回転）
    
    std::cout << "Flag reset on move: OK" << std::endl;
}

void test_flag_reset_on_pair_placement() {
    std::cout << "Testing flag reset on pair placement..." << std::endl;
    
    Field field;
    PuyoController controller(&field);
    
    // テスト用フィールド設定
    PuyoPair pair(PuyoColor::GREEN, PuyoColor::YELLOW, Position(3, 6), Rotation::UP);
    controller.set_current_pair(pair);
    
    // 両側を塞いでフラグ設定状況を作る
    field.set_puyo(Position(2, 6), PuyoColor::RED);
    field.set_puyo(Position(4, 6), PuyoColor::BLUE);
    
    // フラグ設定
    controller.rotate_clockwise();
    
    // ペア配置でフラグリセット
    controller.place_current_pair();
    
    // 新しいペア設定
    PuyoPair new_pair(PuyoColor::GREEN, PuyoColor::PURPLE, Position(3, 6), Rotation::UP);
    controller.set_current_pair(new_pair);
    
    // 回転操作 - 1回目として扱われるべき
    bool result = controller.rotate_clockwise();
    assert(!result);  // フラグがリセットされているので通常回転失敗
    
    std::cout << "Flag reset on pair placement: OK" << std::endl;
}

void test_flag_reset_on_new_pair_set() {
    std::cout << "Testing flag reset on new pair set..." << std::endl;
    
    Field field;
    PuyoController controller(&field);
    
    // フラグ設定状況を作る
    field.set_puyo(Position(2, 6), PuyoColor::RED);
    field.set_puyo(Position(4, 6), PuyoColor::BLUE);
    
    PuyoPair pair(PuyoColor::GREEN, PuyoColor::YELLOW, Position(3, 6), Rotation::UP);
    controller.set_current_pair(pair);
    
    // フラグ設定
    controller.rotate_clockwise();
    
    // 新しいペア設定でフラグリセット
    PuyoPair new_pair(PuyoColor::GREEN, PuyoColor::PURPLE, Position(3, 6), Rotation::UP);
    controller.set_current_pair(new_pair);
    
    // 回転操作 - 1回目として扱われるべき
    bool result = controller.rotate_clockwise();
    assert(!result);  // フラグがリセットされているので通常回転失敗
    
    std::cout << "Flag reset on new pair set: OK" << std::endl;
}

void test_normal_rotation_still_works() {
    std::cout << "Testing normal rotation still works..." << std::endl;
    
    Field field;
    PuyoController controller(&field);
    
    // 通常の回転が可能な状況
    PuyoPair pair(PuyoColor::GREEN, PuyoColor::YELLOW, Position(3, 1), Rotation::UP);
    controller.set_current_pair(pair);
    
    // 通常回転が正常に動作することを確認
    bool result = controller.rotate_clockwise();
    assert(result);
    
    Rotation after_rot = controller.get_current_pair().rot;
    assert(Rotation::RIGHT == after_rot);  // UP → RIGHT
    
    std::cout << "Normal rotation still works: OK" << std::endl;
}

int main() {
    std::cout << "=== Quick Turn Tests ===" << std::endl;
    
    try {
        test_first_rotation_no_rotate_when_blocked();
        test_second_rotation_performs_quick_turn();
        test_flag_reset_on_move();
        test_flag_reset_on_pair_placement();
        test_flag_reset_on_new_pair_set();
        test_normal_rotation_still_works();
        
        std::cout << "\n✅ All quick turn tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}