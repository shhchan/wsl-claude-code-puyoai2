#!/usr/bin/env python3
"""
UI自動テスト
GUI環境なしでもテストできるUI機能確認
"""

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

import puyo_ai_platform as pap
import pygame

def test_ui_components():
    """UIコンポーネントのテスト"""
    print("=== UIコンポーネントテスト ===")
    
    # pygame初期化テスト
    try:
        pygame.init()
        print("✅ pygame初期化: 成功")
    except Exception as e:
        print(f"❌ pygame初期化: 失敗 - {e}")
        return False
    
    # 色定義テスト
    from python.ui.renderer import COLORS
    
    expected_colors = [
        pap.PuyoColor.EMPTY,
        pap.PuyoColor.RED,
        pap.PuyoColor.GREEN,
        pap.PuyoColor.BLUE,
        pap.PuyoColor.YELLOW,
        pap.PuyoColor.PURPLE,
        pap.PuyoColor.GARBAGE
    ]
    
    for color in expected_colors:
        if color in COLORS:
            print(f"✅ 色定義 {color}: {COLORS[color]}")
        else:
            print(f"❌ 色定義 {color}: 未定義")
            return False
    
    # レンダラー作成テスト（実際の画面は開かない）
    try:
        # ヘッドレスモード設定
        os.environ['SDL_VIDEODRIVER'] = 'dummy'
        
        from python.ui.renderer import PuyoRenderer
        renderer = PuyoRenderer()
        print("✅ PuyoRenderer作成: 成功")
        
        # 基本メソッドテスト
        renderer.clear_screen()
        print("✅ clear_screen: 成功")
        
        renderer.quit()
        print("✅ renderer.quit: 成功")
        
    except Exception as e:
        print(f"❌ PuyoRenderer: 失敗 - {e}")
        return False
    
    # InputManagerテスト
    try:
        from python.ui.game_controller import InputManager
        input_manager = InputManager()
        
        # キーマッピングテスト
        test_keys = [
            (pygame.K_a, pap.MoveCommand.LEFT),
            (pygame.K_d, pap.MoveCommand.RIGHT),
            (pygame.K_DOWN, pap.MoveCommand.ROTATE_CCW),
            (pygame.K_RIGHT, pap.MoveCommand.ROTATE_CW),
            (pygame.K_w, pap.MoveCommand.DROP),
        ]
        
        for key, expected_command in test_keys:
            command = input_manager.get_move_command(key)
            if command == expected_command:
                print(f"✅ キーマッピング {key}: {command}")
            else:
                print(f"❌ キーマッピング {key}: 期待値 {expected_command}, 実際 {command}")
                return False
        
        print("✅ InputManager: 成功")
        
    except Exception as e:
        print(f"❌ InputManager: 失敗 - {e}")
        return False
    
    return True

def test_game_controller_logic():
    """ゲームコントローラーロジックテスト"""
    print("\n=== ゲームコントローラーロジックテスト ===")
    
    try:
        # ヘッドレスモード継続
        os.environ['SDL_VIDEODRIVER'] = 'dummy'
        
        from python.ui.game_controller import GameController
        
        # コントローラー作成
        controller = GameController(pap.GameMode.TOKOTON)
        print("✅ GameController作成: 成功")
        
        # セットアップ
        controller.setup_game(["Test Player"])
        print("✅ setup_game: 成功")
        
        # 現在ペア確認
        if controller.current_pair is not None:
            print(f"✅ 初期ぷよペア生成: {controller.current_pair.axis} + {controller.current_pair.child}")
        else:
            print("❌ 初期ぷよペア生成: 失敗")
            return False
        
        # 移動テスト（実際のキー入力ではなく内部メソッド）
        original_x = controller.current_pair.pos.x
        
        # 左移動テスト
        success = controller._try_move_pair(pap.MoveCommand.LEFT)
        if success and controller.current_pair.pos.x == original_x - 1:
            print("✅ 左移動: 成功")
        else:
            print(f"❌ 左移動: 失敗 (原位置:{original_x}, 現位置:{controller.current_pair.pos.x})")
        
        # 右移動テスト（元の位置に戻す）
        success = controller._try_move_pair(pap.MoveCommand.RIGHT)
        if success and controller.current_pair.pos.x == original_x:
            print("✅ 右移動: 成功")
        else:
            print(f"❌ 右移動: 失敗")
        
        # 回転テスト
        original_rot = controller.current_pair.rot
        success = controller._try_move_pair(pap.MoveCommand.ROTATE_CW)
        if success and controller.current_pair.rot != original_rot:
            print("✅ 回転: 成功")
        else:
            print("❌ 回転: 失敗")
        
        # 終了処理
        controller.quit()
        print("✅ controller.quit: 成功")
        
        return True
        
    except Exception as e:
        print(f"❌ GameController: 失敗 - {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    """メインテスト実行"""
    print("UI自動テスト")
    print("=" * 50)
    
    all_passed = True
    
    # UIコンポーネントテスト
    if not test_ui_components():
        all_passed = False
    
    # ゲームコントローラーロジックテスト
    if not test_game_controller_logic():
        all_passed = False
    
    print("\n" + "=" * 50)
    if all_passed:
        print("✅ 全てのテストが成功しました！")
        return 0
    else:
        print("❌ 一部のテストが失敗しました")
        return 1

if __name__ == "__main__":
    sys.exit(main())