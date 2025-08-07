#include "../cpp/core/chain_system.h"
#include "../cpp/core/field.h"
#include <iostream>
#include <cassert>

using namespace puyo;

void test_basic_chain_detection() {
    std::cout << "Testing basic chain detection..." << std::endl;
    
    Field field;
    ChainSystem chain_system(&field);
    
    // 4個の赤ぷよを縦に配置（連鎖になる）
    field.set_puyo(Position(0, 0), PuyoColor::RED);
    field.set_puyo(Position(0, 1), PuyoColor::RED);
    field.set_puyo(Position(0, 2), PuyoColor::RED);
    field.set_puyo(Position(0, 3), PuyoColor::RED);
    
    // 連鎖実行
    ChainSystemResult result = chain_system.execute_chains();
    
    assert(result.has_chains());
    assert(result.total_chains == 1);
    assert(result.chain_results.size() == 1);
    assert(result.chain_results[0].total_cleared == 4);
    assert(result.chain_results[0].color_count == 1);
    
    // デバッグ情報
    std::cout << "Chain level: " << result.chain_results[0].chain_level << std::endl;
    std::cout << "Total cleared: " << result.chain_results[0].total_cleared << std::endl;
    std::cout << "Color count: " << result.chain_results[0].color_count << std::endl;
    std::cout << "Chain score: " << result.score_result.chain_score << std::endl;
    std::cout << "Expected: 40" << std::endl;
    
    // 1連鎖4個消しの特例スコア（40点）
    assert(result.score_result.chain_score == 40);
    
    std::cout << "Basic chain detection: OK" << std::endl;
}

void test_multi_chain() {
    std::cout << "Testing multi-chain..." << std::endl;
    
    Field field;
    ChainSystem chain_system(&field);
    
    // 2連鎖を設定
    // 1連鎖目：赤4個（縦）
    field.set_puyo(Position(0, 0), PuyoColor::RED);
    field.set_puyo(Position(0, 1), PuyoColor::RED);
    field.set_puyo(Position(0, 2), PuyoColor::RED);
    field.set_puyo(Position(0, 3), PuyoColor::RED);
    
    // 2連鎖目：青4個（1連鎖後に落下して形成）
    field.set_puyo(Position(1, 0), PuyoColor::BLUE);
    field.set_puyo(Position(1, 1), PuyoColor::BLUE);
    field.set_puyo(Position(1, 2), PuyoColor::BLUE);
    field.set_puyo(Position(1, 4), PuyoColor::BLUE);  // 1段空けて配置
    
    // 連鎖実行
    ChainSystemResult result = chain_system.execute_chains();
    
    assert(result.has_chains());
    assert(result.total_chains == 2);
    
    // 1連鎖目：40点（特例）
    // 2連鎖目：4 × (8 + 0 + 0) × 10 = 320点
    int expected_score = 40 + 320;
    assert(result.score_result.chain_score == expected_score);
    
    std::cout << "Multi-chain: OK" << std::endl;
}

void test_color_bonus() {
    std::cout << "Testing color bonus..." << std::endl;
    
    Field field;
    ChainSystem chain_system(&field);
    
    // 2色同時消し
    // 赤4個
    field.set_puyo(Position(0, 0), PuyoColor::RED);
    field.set_puyo(Position(0, 1), PuyoColor::RED);
    field.set_puyo(Position(0, 2), PuyoColor::RED);
    field.set_puyo(Position(0, 3), PuyoColor::RED);
    
    // 青4個（隣接）
    field.set_puyo(Position(1, 0), PuyoColor::BLUE);
    field.set_puyo(Position(1, 1), PuyoColor::BLUE);
    field.set_puyo(Position(1, 2), PuyoColor::BLUE);
    field.set_puyo(Position(1, 3), PuyoColor::BLUE);
    
    // 連鎖実行
    ChainSystemResult result = chain_system.execute_chains();
    
    assert(result.has_chains());
    assert(result.total_chains == 1);
    assert(result.chain_results[0].total_cleared == 8);
    assert(result.chain_results[0].color_count == 2);
    
    // 8 × (0 + 0 + 3) × 10 = 240点
    assert(result.score_result.chain_score == 240);
    
    std::cout << "Color bonus: OK" << std::endl;
}

void test_connection_bonus() {
    std::cout << "Testing connection bonus..." << std::endl;
    
    Field field;
    ChainSystem chain_system(&field);
    
    // 5個の赤ぷよをL字型に配置
    field.set_puyo(Position(0, 0), PuyoColor::RED);
    field.set_puyo(Position(0, 1), PuyoColor::RED);
    field.set_puyo(Position(0, 2), PuyoColor::RED);
    field.set_puyo(Position(1, 0), PuyoColor::RED);
    field.set_puyo(Position(2, 0), PuyoColor::RED);
    
    // 連鎖実行
    ChainSystemResult result = chain_system.execute_chains();
    
    assert(result.has_chains());
    assert(result.total_chains == 1);
    assert(result.chain_results[0].total_cleared == 5);
    
    // 5 × (0 + 2 + 0) × 10 = 100点（連結ボーナス2）
    assert(result.score_result.chain_score == 100);
    
    std::cout << "Connection bonus: OK" << std::endl;
}

void test_all_clear() {
    std::cout << "Testing all clear..." << std::endl;
    
    Field field;
    ChainSystem chain_system(&field);
    
    // フィールドに少しだけぷよを置く
    field.set_puyo(Position(0, 0), PuyoColor::RED);
    field.set_puyo(Position(0, 1), PuyoColor::RED);
    field.set_puyo(Position(0, 2), PuyoColor::RED);
    field.set_puyo(Position(0, 3), PuyoColor::RED);
    
    // 連鎖実行（全て消える）
    ChainSystemResult result = chain_system.execute_chains();
    
    assert(result.score_result.is_all_clear);
    
    // 次回連鎖で全消しボーナスが付くことを確認
    assert(chain_system.get_score_calculator().get_pending_all_clear_bonus() == 2100);
    
    std::cout << "All clear: OK" << std::endl;
}

void test_drop_bonus() {
    std::cout << "Testing drop bonus..." << std::endl;
    
    Field field;
    ChainSystem chain_system(&field);
    
    // 4個の赤ぷよを配置
    field.set_puyo(Position(0, 0), PuyoColor::RED);
    field.set_puyo(Position(0, 1), PuyoColor::RED);
    field.set_puyo(Position(0, 2), PuyoColor::RED);
    field.set_puyo(Position(0, 3), PuyoColor::RED);
    
    // 落下距離5段での連鎖実行
    int drop_height = 5;
    ChainSystemResult result = chain_system.execute_chains_with_drop_bonus(drop_height);
    
    // 落下ボーナス：5 + 1 = 6点
    assert(result.score_result.drop_score == 6);
    
    // 総得点：40（連鎖） + 6（落下） = 46点
    assert(result.score_result.total_score == 46);
    
    std::cout << "Drop bonus: OK" << std::endl;
}

void test_chain_prediction() {
    std::cout << "Testing chain prediction..." << std::endl;
    
    Field field;
    ChainSystem chain_system(&field);
    
    // 連鎖が発生しない状態
    field.set_puyo(Position(0, 0), PuyoColor::RED);
    field.set_puyo(Position(0, 1), PuyoColor::RED);
    field.set_puyo(Position(0, 2), PuyoColor::BLUE);
    
    assert(!chain_system.would_cause_chain());
    assert(chain_system.count_potential_chains() == 0);
    
    // 連鎖が発生する状態に変更
    field.set_puyo(Position(0, 2), PuyoColor::RED);
    field.set_puyo(Position(0, 3), PuyoColor::RED);
    
    assert(chain_system.would_cause_chain());
    assert(chain_system.count_potential_chains() == 1);
    
    std::cout << "Chain prediction: OK" << std::endl;
}

int main() {
    std::cout << "=== Chain System Tests ===" << std::endl;
    
    try {
        test_basic_chain_detection();
        test_multi_chain();
        test_color_bonus();
        test_connection_bonus();
        test_all_clear();
        test_drop_bonus();
        test_chain_prediction();
        
        std::cout << "\n✅ All chain system tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}