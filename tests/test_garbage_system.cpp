#include "../cpp/core/garbage_system.h"
#include "../cpp/core/chain_system.h"
#include "../cpp/core/field.h"
#include <iostream>
#include <cassert>

using namespace puyo;

void test_garbage_calculation_with_accumulation() {
    std::cout << "Testing garbage calculation with score accumulation..." << std::endl;
    
    Field field;
    GarbageSystem garbage_system(&field);
    
    // 150点のスコア -> 2個のおじゃま + 10点蓄積
    int garbage1 = garbage_system.calculate_garbage_to_send(150);
    assert(garbage1 == 2);
    assert(garbage_system.get_accumulated_score() == 10);
    
    // 80点のスコア + 10点蓄積 = 90点 -> 1個のおじゃま + 20点蓄積
    int garbage2 = garbage_system.calculate_garbage_to_send(80);
    assert(garbage2 == 1);
    assert(garbage_system.get_accumulated_score() == 20);
    
    // 50点のスコア + 20点蓄積 = 70点 -> 1個のおじゃま + 0点蓄積
    int garbage3 = garbage_system.calculate_garbage_to_send(50);
    assert(garbage3 == 1);
    assert(garbage_system.get_accumulated_score() == 0);
    
    std::cout << "Garbage calculation with accumulation: OK" << std::endl;
}

void test_garbage_offset() {
    std::cout << "Testing garbage offset system..." << std::endl;
    
    Field field;
    GarbageSystem garbage_system(&field);
    
    // 予告おじゃまぷよを追加
    garbage_system.add_pending_garbage(5);
    garbage_system.add_pending_garbage(3);
    assert(garbage_system.get_pending_garbage_count() == 8);
    
    // 210点 (3個分) で相殺
    int offset_count = garbage_system.offset_garbage_with_score(210);
    assert(offset_count == 3);
    assert(garbage_system.get_pending_garbage_count() == 5);
    
    // 350点 (5個分) で完全相殺
    offset_count = garbage_system.offset_garbage_with_score(350);
    assert(offset_count == 5);
    assert(garbage_system.get_pending_garbage_count() == 0);
    
    std::cout << "Garbage offset system: OK" << std::endl;
}

void test_garbage_placement_pattern() {
    std::cout << "Testing garbage placement pattern (N layers + remainder)..." << std::endl;
    
    Field field;
    GarbageSystem garbage_system(&field);
    
    // 20個のおじゃまぷよを配置 (6 * 3 + 2 = 3段 + 2個)
    garbage_system.add_pending_garbage(20);
    GarbageResult result = garbage_system.drop_pending_garbage();
    
    assert(result.placed_garbage == 20);
    
    // フィールドでのおじゃまぷよ配置を確認
    int garbage_count = 0;
    for (int x = 0; x < FIELD_WIDTH; ++x) {
        for (int y = 0; y < FIELD_HEIGHT; ++y) {
            if (field.get_puyo(Position(x, y)) == PuyoColor::GARBAGE) {
                garbage_count++;
            }
        }
    }
    
    assert(garbage_count == 20);
    
    // 最下段3行が全ておじゃまぷよで、4段目に2個のおじゃまぷよがあることを確認
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            assert(field.get_puyo(Position(x, y)) == PuyoColor::GARBAGE);
        }
    }
    
    // 4段目には2個のおじゃまぷよがある
    int fourth_row_garbage = 0;
    for (int x = 0; x < FIELD_WIDTH; ++x) {
        if (field.get_puyo(Position(x, 3)) == PuyoColor::GARBAGE) {
            fourth_row_garbage++;
        }
    }
    assert(fourth_row_garbage == 2);
    
    std::cout << "Garbage placement pattern: OK" << std::endl;
}

void test_garbage_chain_interaction() {
    std::cout << "Testing garbage puyo chain interaction..." << std::endl;
    
    Field field;
    ChainSystem chain_system(&field);
    
    // 連鎖可能な色ぷよを配置
    field.set_puyo(Position(0, 0), PuyoColor::RED);
    field.set_puyo(Position(0, 1), PuyoColor::RED);
    field.set_puyo(Position(0, 2), PuyoColor::RED);
    field.set_puyo(Position(0, 3), PuyoColor::RED);
    
    // 隣接におじゃまぷよを配置
    field.set_puyo(Position(1, 0), PuyoColor::GARBAGE);
    field.set_puyo(Position(1, 1), PuyoColor::GARBAGE);
    
    // 連鎖実行（おじゃまぷよも巻き込まれるはず）
    ChainSystemResult result = chain_system.execute_chains();
    
    assert(result.has_chains());
    assert(result.total_chains == 1);
    
    // おじゃまぷよが消えたことを確認
    assert(field.get_puyo(Position(1, 0)) == PuyoColor::EMPTY);
    assert(field.get_puyo(Position(1, 1)) == PuyoColor::EMPTY);
    
    // 色ぷよも消えたことを確認
    assert(field.get_puyo(Position(0, 0)) == PuyoColor::EMPTY);
    assert(field.get_puyo(Position(0, 1)) == PuyoColor::EMPTY);
    assert(field.get_puyo(Position(0, 2)) == PuyoColor::EMPTY);
    assert(field.get_puyo(Position(0, 3)) == PuyoColor::EMPTY);
    
    std::cout << "Garbage puyo chain interaction: OK" << std::endl;
}

void test_garbage_non_chain_property() {
    std::cout << "Testing garbage puyo non-chain property..." << std::endl;
    
    Field field;
    ChainSystem chain_system(&field);
    
    // 4個のおじゃまぷよを隣接配置（連鎖にならないはず）
    field.set_puyo(Position(0, 0), PuyoColor::GARBAGE);
    field.set_puyo(Position(0, 1), PuyoColor::GARBAGE);
    field.set_puyo(Position(1, 0), PuyoColor::GARBAGE);
    field.set_puyo(Position(1, 1), PuyoColor::GARBAGE);
    
    // 連鎖検出（おじゃまぷよだけでは連鎖しない）
    ChainSystemResult result = chain_system.execute_chains();
    
    assert(!result.has_chains());
    assert(result.total_chains == 0);
    
    // おじゃまぷよがそのまま残っていることを確認
    assert(field.get_puyo(Position(0, 0)) == PuyoColor::GARBAGE);
    assert(field.get_puyo(Position(0, 1)) == PuyoColor::GARBAGE);
    assert(field.get_puyo(Position(1, 0)) == PuyoColor::GARBAGE);
    assert(field.get_puyo(Position(1, 1)) == PuyoColor::GARBAGE);
    
    std::cout << "Garbage puyo non-chain property: OK" << std::endl;
}

void test_complex_garbage_scenario() {
    std::cout << "Testing complex garbage scenario..." << std::endl;
    
    Field field;
    GarbageSystem garbage_system(&field);
    
    // 複数回のスコア計算とおじゃまぷよ送信
    int total_sent = 0;
    
    // 1回目: 85点 -> 1個 + 15点蓄積
    total_sent += garbage_system.calculate_garbage_to_send(85);
    
    // 2回目: 55点 + 15点蓄積 = 70点 -> 1個 + 0点蓄積  
    total_sent += garbage_system.calculate_garbage_to_send(55);
    
    // 3回目: 140点 -> 2個 + 0点蓄積
    total_sent += garbage_system.calculate_garbage_to_send(140);
    
    assert(total_sent == 4);
    assert(garbage_system.get_accumulated_score() == 0);
    
    // 予告おじゃまぷよの追加と一部相殺
    garbage_system.add_pending_garbage(10);
    assert(garbage_system.get_pending_garbage_count() == 10);
    
    // 280点で4個相殺
    int offset = garbage_system.offset_garbage_with_score(280);
    assert(offset == 4);
    assert(garbage_system.get_pending_garbage_count() == 6);
    
    // 残りを降下
    GarbageResult result = garbage_system.drop_pending_garbage();
    assert(result.placed_garbage == 6);
    
    std::cout << "Complex garbage scenario: OK" << std::endl;
}

int main() {
    std::cout << "=== Garbage System Tests ===" << std::endl;
    
    try {
        test_garbage_calculation_with_accumulation();
        test_garbage_offset();
        test_garbage_placement_pattern();
        test_garbage_chain_interaction();
        test_garbage_non_chain_property();
        test_complex_garbage_scenario();
        
        std::cout << "\n✅ All garbage system tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}