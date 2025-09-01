#!/usr/bin/env python3
"""
AI Mode統合テスト - UIのAI Modeが正常動作することを確認
"""
import sys
import os

# プロジェクトルートをパスに追加
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import puyo_ai_platform as pap
from python.ui.player_controller import AIPlayerController

def test_ai_player_controller():
    """AIPlayerControllerの動作テスト"""
    print("=== AIPlayerController動作テスト ===")
    
    # RandomAIインスタンスを作成
    ai_manager = pap.ai.get_global_ai_manager()
    ai_instance = ai_manager.create_ai("random", {"seed": "12345"})
    
    if ai_instance is None:
        print("ERROR: RandomAI作成失敗")
        return False
    
    # AIPlayerControllerを作成
    controller = AIPlayerController(ai_instance, "Test AI Player")
    print(f"AIPlayerController作成成功: {controller.get_player_name()}")
    
    # ゲーム状態をシミュレート
    field = pap.Field()
    player = None  # ここではNoneで代用
    
    # 簡単なゲーム状態を構築
    game_state = {
        'current_player': None,  # 最初はNoneでテスト
        'current_pair': pap.PuyoPair(pap.PuyoColor.RED, pap.PuyoColor.BLUE),
        'turn_count': 1,
        'pair_placed': False,
        'last_chain_count': 0,
        'last_score_details': None
    }
    
    # AIPlayerControllerからコマンドを取得
    print("\n--- Fieldなしでのテスト ---")
    try:
        command = controller.get_next_command(game_state)
        print(f"取得されたコマンド: {command}")
        
        # エラーハンドリングが適切に動作することを確認
        if command:
            print("コマンド取得成功（エラーハンドリング経由かもしれません）")
        else:
            print("コマンドなし（正常）")
    except Exception as e:
        print(f"予期しないエラー: {e}")
        return False
    
    print("\n--- 改善されたテスト（GameManagerを使用） ---")
    try:
        # より現実的なテスト環境を構築
        game_manager = pap.GameManager(pap.GameMode.TOKOTON)
        game_manager.add_player("Test Player", pap.PlayerType.HUMAN)
        player = game_manager.get_player(0)
        
        game_state_with_player = {
            'current_player': player,
            'current_pair': pap.PuyoPair(pap.PuyoColor.GREEN, pap.PuyoColor.YELLOW),
            'turn_count': 1,
            'pair_placed': False,
            'last_chain_count': 0,
            'last_score_details': None
        }
        
        # AIPlayerControllerからコマンドを取得
        command = controller.get_next_command(game_state_with_player)
        print(f"取得されたコマンド: {command}")
        
        if command:
            print("✅ AIからコマンド取得成功")
            
            # 連続でコマンドを取得してキュー機能をテスト
            for i in range(5):
                next_command = controller.get_next_command(game_state_with_player)
                if next_command:
                    print(f"  キューコマンド {i+1}: {next_command}")
                else:
                    print(f"  キューコマンド {i+1}: なし（正常終了）")
                    break
        else:
            print("⚠️ コマンドなし（思考間隔制御または他の理由）")
            
    except Exception as e:
        print(f"エラー: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    print("AIPlayerController動作テスト: OK")
    return True

def test_randomai_directly():
    """RandomAIの直接動作テスト"""
    print("\n=== RandomAI直接動作テスト ===")
    
    # RandomAIを直接テスト
    ai_manager = pap.ai.get_global_ai_manager()
    ai_instance = ai_manager.create_ai("random", {"seed": "98765"})
    ai_instance.initialize()
    
    # GameStateを構築
    game_manager = pap.GameManager(pap.GameMode.TOKOTON)
    game_manager.add_player("Test Player", pap.PlayerType.HUMAN)
    player = game_manager.get_player(0)
    
    game_state = pap.ai.GameState()
    game_state.player_id = 0
    game_state.turn_count = 1
    game_state.is_versus_mode = False
    game_state.current_pair = pap.PuyoPair(pap.PuyoColor.RED, pap.PuyoColor.BLUE)
    game_state.set_own_field(player.get_field())
    
    try:
        # AI思考実行
        decision = ai_instance.think(game_state)
        
        print(f"AIDecision.x: {decision.x}")
        print(f"AIDecision.r: {decision.r}")
        print(f"move_commands: {len(decision.move_commands)}")
        print(f"confidence: {decision.confidence}")
        print(f"reason: {decision.reason}")
        
        # 新しい構造体のフィールドが存在することを確認
        assert hasattr(decision, 'x'), "x field missing"
        assert hasattr(decision, 'r'), "r field missing"
        assert hasattr(decision, 'move_commands'), "move_commands field missing"
        assert hasattr(decision, 'confidence'), "confidence field missing"
        assert hasattr(decision, 'reason'), "reason field missing"
        assert len(decision.move_commands) > 0, "move_commands should not be empty"
        
        print("✅ RandomAI直接動作テスト: OK")
        return True
        
    except Exception as e:
        print(f"エラー: {e}")
        import traceback
        traceback.print_exc()
        return False

def run_all_tests():
    """全テストの実行"""
    print("AI Mode Integration Tests")
    print("=" * 50)
    
    tests = [
        test_randomai_directly,
        test_ai_player_controller
    ]
    
    passed = 0
    for test in tests:
        try:
            if test():
                passed += 1
            print()  # 空行を追加
        except Exception as e:
            print(f"Test {test.__name__} failed: {e}")
    
    print(f"総合結果: {passed}/{len(tests)} テスト通過")
    return passed == len(tests)

if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)