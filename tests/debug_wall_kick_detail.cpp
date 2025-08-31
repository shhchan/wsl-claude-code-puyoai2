#include <iostream>
#include "../cpp/core/field.h"
#include "../cpp/core/puyo_controller.h"
#include "../cpp/core/puyo_types.h"

using namespace puyo;

int main() {
    Field field;
    PuyoController controller(&field);
    
    // テストと同じ状況を再現
    field.clear();
    
    // 14段目にぷよを配置
    field.set_puyo(Position(3, 13), PuyoColor::RED);  // 4列目14段目
    field.mark_row14_used(3);  // 14段目使用フラグを立てる
    
    // 4列目に組ぷよを配置（左向き）
    PuyoPair pair;
    pair.axis = PuyoColor::BLUE;
    pair.child = PuyoColor::YELLOW;
    pair.pos = Position(3, 12);  // 4列目13段目
    pair.rot = Rotation::LEFT;   // 左向き
    
    controller.set_current_pair(pair);
    
    PuyoPair before = controller.get_current_pair();
    Position before_child = before.get_child_position();
    std::cout << "テスト状況での回転前の位置:" << std::endl;
    std::cout << "  軸ぷよ: x=" << before.pos.x << ", y=" << before.pos.y << std::endl;
    std::cout << "  子ぷよ: x=" << before_child.x << ", y=" << before_child.y << std::endl;
    std::cout << "  回転: " << static_cast<int>(before.rot) << " (LEFT)" << std::endl;
    
    // 時計回りの回転（左→上）を実行
    bool result = controller.rotate_clockwise();
    std::cout << "回転成功: " << (result ? "true" : "false") << std::endl;
    
    PuyoPair after = controller.get_current_pair();
    Position after_child = after.get_child_position();
    std::cout << "回転後の位置:" << std::endl;
    std::cout << "  軸ぷよ: x=" << after.pos.x << ", y=" << after.pos.y << std::endl;
    std::cout << "  子ぷよ: x=" << after_child.x << ", y=" << after_child.y << std::endl;
    std::cout << "  回転: " << static_cast<int>(after.rot) << std::endl;
    
    std::cout << "\n位置変化:" << std::endl;
    std::cout << "  軸ぷよ: (" << before.pos.x << "," << before.pos.y << ") → (" 
              << after.pos.x << "," << after.pos.y << ")" << std::endl;
    std::cout << "  子ぷよ: (" << before_child.x << "," << before_child.y << ") → (" 
              << after_child.x << "," << after_child.y << ")" << std::endl;
    
    std::cout << "\nフィールド状況:\n" << field.to_string() << std::endl;
    
    return 0;
}