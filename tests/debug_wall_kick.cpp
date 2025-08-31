#include <iostream>
#include "../cpp/core/field.h"
#include "../cpp/core/puyo_controller.h"
#include "../cpp/core/puyo_types.h"

using namespace puyo;

int main() {
    Field field;
    PuyoController controller(&field);
    
    // 14段目の2列目にぷよを配置（子ぷよが上に移動する先を塞ぐ）
    field.set_puyo(Position(1, 13), PuyoColor::RED);  // 2列目14段目
    
    // 2列目に組ぷよを配置（左向き）
    PuyoPair pair;
    pair.axis = PuyoColor::BLUE;
    pair.child = PuyoColor::YELLOW;
    pair.pos = Position(1, 12);  // 2列目13段目
    pair.rot = Rotation::LEFT;   // 左向き（子ぷよは1列目）
    
    controller.set_current_pair(pair);
    
    PuyoPair before = controller.get_current_pair();
    Position before_child = before.get_child_position();
    std::cout << "回転前の位置:" << std::endl;
    std::cout << "  軸ぷよ: x=" << before.pos.x << ", y=" << before.pos.y << std::endl;
    std::cout << "  子ぷよ: x=" << before_child.x << ", y=" << before_child.y << std::endl;
    std::cout << "  回転: " << static_cast<int>(before.rot) << " (LEFT)" << std::endl;
    
    // 時計回りの回転（左→上）を実行
    bool result = controller.rotate_clockwise();
    std::cout << "回転成功: " << (result ? "true" : "false") << std::endl;
    
    PuyoPair after = controller.get_current_pair();
    std::cout << "回転後の位置: x=" << after.pos.x 
              << ", y=" << after.pos.y 
              << ", rot=" << static_cast<int>(after.rot) << std::endl;
    
    std::cout << "\nフィールド状況:\n" << field.to_string() << std::endl;
    
    return 0;
}