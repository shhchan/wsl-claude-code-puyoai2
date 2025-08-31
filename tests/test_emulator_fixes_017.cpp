#include <iostream>
#include <cassert>
#include "../cpp/core/field.h"
#include "../cpp/core/puyo_controller.h"
#include "../cpp/core/puyo_types.h"

using namespace puyo;

// テスト用のヘルパー関数
void print_test_result(const std::string& test_name, bool result) {
    std::cout << "[" << (result ? "PASS" : "FAIL") << "] " << test_name << std::endl;
}

// 壁キック機能のテスト
void test_wall_kick_upward_rotation() {
    std::cout << "\n=== 壁キック機能のテスト ===" << std::endl;
    
    Field field;
    PuyoController controller(&field);
    
    // テスト1: 14段目にぷよがある状況での上への回転（時計回り）
    {
        field.clear();
        // 14段目にぷよを配置
        field.set_puyo(Position(3, 13), PuyoColor::RED);  // 4列目14段目
        // 14段目にぷよを配置したので，14段目使用フラグを立てる
        field.mark_row14_used(3);
        
        // 4列目に組ぷよを配置（左向き）
        PuyoPair pair;
        pair.axis = PuyoColor::BLUE;
        pair.child = PuyoColor::YELLOW;
        pair.pos = Position(3, 12);  // 4列目13段目
        pair.rot = Rotation::LEFT;   // 左向き
        
        controller.set_current_pair(pair);
        
        // 時計回りの回転（左→上）を実行
        bool result = controller.rotate_clockwise();
        
        // 上にぷよがあるので壁キックでy方向に-1移動する
        print_test_result("壁キック上回転（時計回り）", result);
        
        // 位置が下に移動しているかチェック
        PuyoPair current = controller.get_current_pair();
        bool position_moved = (current.pos.y == 11);  // 12→11に移動
        print_test_result("壁キック位置移動確認", position_moved);
    }
    
    // テスト2: 14段目にぷよがある状況での上への回転（反時計回り）
    {
        field.clear();
        // 14段目にぷよを配置
        field.set_puyo(Position(3, 13), PuyoColor::RED);  // 4列目14段目
        // 14段目にぷよを配置したので，14段目使用フラグを立てる
        field.mark_row14_used(3);

        // 3列目に組ぷよを配置（右向き）
        PuyoPair pair;
        pair.axis = PuyoColor::BLUE;
        pair.child = PuyoColor::YELLOW;
        pair.pos = Position(3, 12);  // 4列目13段目
        pair.rot = Rotation::RIGHT;  // 右向き
        
        controller.set_current_pair(pair);
        
        // 反時計回りの回転（右→上）を実行
        bool result = controller.rotate_counter_clockwise();
        
        // 上にぷよがあるので壁キックでy方向に-1移動する
        print_test_result("壁キック上回転（反時計回り）", result);
        
        // 位置が下に移動しているかチェック
        PuyoPair current = controller.get_current_pair();
        bool position_moved = (current.pos.y == 11);  // 12→11に移動
        print_test_result("壁キック位置移動確認（反時計回り）", position_moved);
    }
    
    // テスト3: 14段目にぷよがない場合は通常の回転
    {
        field.clear();
        
        // 3列目に組ぷよを配置（左向き）
        PuyoPair pair;
        pair.axis = PuyoColor::BLUE;
        pair.child = PuyoColor::YELLOW;
        pair.pos = Position(2, 10);  // 3列目11段目（十分下）
        pair.rot = Rotation::LEFT;   // 左向き
        
        controller.set_current_pair(pair);
        
        // 時計回りの回転（左→上）を実行
        bool result = controller.rotate_clockwise();
        print_test_result("通常回転（壁キック不要）", result);
        
        // 位置が移動していないかチェック
        PuyoPair current = controller.get_current_pair();
        bool position_unchanged = (current.pos.x == 2 && current.pos.y == 10);  // 位置は変わらず
        print_test_result("通常回転位置不変確認", position_unchanged);
    }
}

// 設置可能判定のテスト
void test_can_place_algorithm() {
    std::cout << "\n=== 設置可能判定のテスト ===" << std::endl;
    
    Field field;
    
    // テスト1: 空のフィールドでの基本配置
    {
        field.clear();
        
        // すべての位置・回転で配置可能であることを確認
        // 1列目の回転状態左，6列目の回転状態右は除外
        bool all_positions_valid = true;
        for (int x = 0; x < 6; ++x) {
            for (int r = 0; r < 4; ++r) {
                if (x == 0 && r == 3) continue; // 1列目左向きは除外
                if (x == 5 && r == 1) continue; // 6列目右向きは除外
                if (!field.can_place(x, r)) {
                    all_positions_valid = false;
                    break;
                }
            }
        }
        print_test_result("空フィールド全位置配置可能", all_positions_valid);
    }
    
    // テスト2: 高さ制限のテスト
    {
        field.clear();
        
        // 1列目を12段目まで埋める
        for (int y = 0; y < 12; ++y) {
            field.set_puyo(Position(0, y), PuyoColor::RED);
        }
        
        // 1列目（x=0）での配置をテスト
        bool cannot_place_when_high = !field.can_place(0, 0);  // 上向き
        print_test_result("高さ制限テスト（配置不可）", cannot_place_when_high);
    }
    
    // テスト3: 14段目制限のテスト
    {
        field.clear();
        
        // 1列目を13段目まで埋める
        for (int y = 0; y < 13; ++y) {
            field.set_puyo(Position(0, y), PuyoColor::RED);
        }
        // 14段目にも配置
        field.set_puyo(Position(0, 13), PuyoColor::BLUE);
        field.mark_row14_used(0);
        
        // 1列目での配置テスト
        bool cannot_place_row14 = !field.can_place(0, 0);
        print_test_result("14段目制限テスト", cannot_place_row14);
    }
    
    // テスト4: 複雑な配置パターンのテスト
    {
        field.clear();
        
        // 複雑な形状を作成
        // 2列目を12段目まで埋める
        for (int y = 0; y < 12; ++y) {
            field.set_puyo(Position(1, y), PuyoColor::RED);
        }
        // 4列目を12段目まで埋める
        for (int y = 0; y < 12; ++y) {
            field.set_puyo(Position(3, y), PuyoColor::BLUE);
        }
        
        // 4列目での配置をテスト（2列目と4列目が12段）
        bool can_place_center = field.can_place(3, 0);  // 上向き
        print_test_result("複雑パターンでの配置可能", can_place_center);
    }
    
    // テスト5: PuyoPairでの統合テスト
    {
        field.clear();
        
        PuyoPair pair;
        pair.axis = PuyoColor::RED;
        pair.child = PuyoColor::BLUE;
        pair.pos = Position(2, 12);
        pair.rot = Rotation::UP;
        
        bool can_place_pair = field.can_place_puyo_pair(pair);
        print_test_result("PuyoPair統合テスト", can_place_pair);
    }
}

// 回帰テスト（既存機能が破綻していないか）
void test_regression() {
    std::cout << "\n=== 回帰テスト ===" << std::endl;
    
    Field field;
    PuyoController controller(&field);
    
    // 基本的な配置・落下が正常に動作するか
    {
        field.clear();
        
        PuyoPair pair;
        pair.axis = PuyoColor::RED;
        pair.child = PuyoColor::BLUE;
        pair.pos = Position(2, 10);
        pair.rot = Rotation::UP;
        
        controller.set_current_pair(pair);
        bool placed = controller.place_current_pair();
        print_test_result("基本的なぷよ配置", placed);
        
        // 配置されたぷよが正しい位置にあるか
        bool axis_placed = (field.get_puyo(Position(2, 10)) == PuyoColor::RED);
        bool child_placed = (field.get_puyo(Position(2, 11)) == PuyoColor::BLUE);
        print_test_result("配置位置確認", axis_placed && child_placed);
    }
    
    // 移動機能が正常に動作するか
    {
        field.clear();
        
        PuyoPair pair;
        pair.axis = PuyoColor::YELLOW;
        pair.child = PuyoColor::GREEN;
        pair.pos = Position(2, 10);
        pair.rot = Rotation::UP;
        
        controller.set_current_pair(pair);
        
        bool moved_left = controller.move_left();
        bool moved_right = controller.move_right();
        bool moved_down = controller.move_down();
        
        print_test_result("基本移動機能", moved_left && moved_right && moved_down);
    }
}

int main() {
    std::cout << "チケット017: ぷよぷよエミュレータ微修正テスト開始" << std::endl;
    
    test_wall_kick_upward_rotation();
    test_can_place_algorithm();
    test_regression();
    
    std::cout << "\n全テスト完了" << std::endl;
    return 0;
}