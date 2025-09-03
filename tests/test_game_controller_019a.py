#!/usr/bin/env python3
"""
ChainSearchAIとgame_controller.pyの統合テスト
"""
import sys
import os

# プロジェクトルートをパスに追加
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import puyo_ai_platform as pap
from python.ui.game_controller import GameController
from python.ui.player_controller import AIPlayerController

def test_chain_search_ai_game_integration():
    """ChainSearchAIとGameControllerの統合テスト"""
    print("=== ChainSearchAI GameController統合テスト ===")
    
    try:
        # AIマネージャーからChainSearchAIを作成
        ai_manager = pap.ai.get_global_ai_manager()
        ai_names = ai_manager.get_registered_ai_names()
        print(f"利用可能AI: {ai_names}")
        
        if "chain_search" not in ai_names:
            print("ERROR: ChainSearchAI not available")
            return False
        
        # ChainSearchAIインスタンス作成
        chain_ai = ai_manager.create_ai("chain_search")
        if chain_ai is None:
            print("ERROR: Failed to create ChainSearchAI")
            return False
        
        # AIPlayerController作成
        ai_controller = AIPlayerController(chain_ai, "ChainSearchAI Test")
        print(f"AIPlayerController作成成功: {ai_controller.get_player_name()}")
        
        # GameController作成（UIなしでテスト）
        game_controller = GameController(pap.GameMode.TOKOTON, ai_controller)
        
        # モック化されたsetup_game（プレイヤー名設定のみ）
        try:
            game_controller.setup_game([ai_controller.get_player_name()])
            print("ゲームセットアップ成功")
        except Exception as e:
            print(f"ゲームセットアップエラー: {e}")
            return False
        
        # AI制御モードが有効かチェック
        if not game_controller.ai_mode:
            print("ERROR: AI制御モードが有効になっていません")
            return False
        
        print("✅ AI制御モード有効")
        
        # 複数回のアップデートをシミュレーション（実際のゲームループの代替）
        update_count = 0
        max_updates = 10  # 10回のアップデートでテスト終了
        
        while update_count < max_updates:
            try:
                # ゲーム状態のアップデート
                continue_game = game_controller.update()
                
                if not continue_game:
                    print(f"ゲーム終了が検出されました (アップデート{update_count+1}回目)")
                    break
                
                update_count += 1
                
                if update_count % 3 == 0:
                    print(f"アップデート {update_count}/10 完了")
                
            except Exception as e:
                print(f"ゲームアップデートエラー: {e}")
                import traceback
                traceback.print_exc()
                return False
        
        print(f"✅ {update_count}回のゲームアップデートが正常に完了")
        
        # 終了処理
        try:
            game_controller.quit()
            print("✅ ゲーム終了処理完了")
        except Exception as e:
            print(f"終了処理エラー: {e}")
        
        return True
        
    except Exception as e:
        print(f"統合テストでエラー発生: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_ai_comparison():
    """RandomAIとChainSearchAIの比較テスト"""
    print("\n=== RandomAI vs ChainSearchAI 比較テスト ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    
    # テスト用のゲーム状態を作成
    gm = pap.GameManager(pap.GameMode.TOKOTON)
    gm.add_player('Test Player', pap.PlayerType.HUMAN)
    player = gm.get_player(0)
    field = player.get_field()
    
    game_state = pap.ai.GameState()
    game_state.player_id = 0
    game_state.turn_count = 1
    game_state.is_versus_mode = False
    game_state.current_pair = pap.PuyoPair(pap.PuyoColor.RED, pap.PuyoColor.BLUE)
    game_state.set_own_field(field)
    
    # RandomAIの判定テスト
    random_ai = ai_manager.create_ai("random")
    random_ai.initialize()
    
    print("RandomAI decisions:")
    for i in range(3):
        decision = random_ai.think(game_state)
        print(f"  {i+1}: ({decision.x}, {decision.r}) 確信度={decision.confidence:.3f}")
    
    # ChainSearchAIの判定テスト
    chain_ai = ai_manager.create_ai("chain_search")
    chain_ai.initialize()
    
    print("ChainSearchAI decisions:")
    for i in range(3):
        decision = chain_ai.think(game_state)
        print(f"  {i+1}: ({decision.x}, {decision.r}) 確信度={decision.confidence:.3f} スコア評価による")
    
    # 思考時間比較
    import time
    
    # RandomAI思考時間測定
    start_time = time.time()
    random_ai.think(game_state)
    random_time = time.time() - start_time
    
    # ChainSearchAI思考時間測定
    start_time = time.time()
    chain_ai.think(game_state)
    chain_time = time.time() - start_time
    
    print(f"\n思考時間比較:")
    print(f"  RandomAI: {random_time*1000:.2f}ms")
    print(f"  ChainSearchAI: {chain_time*1000:.2f}ms")
    print(f"  ChainSearchAIはRandomAIの約{chain_time/random_time:.1f}倍の思考時間")
    
    return True

def run_all_tests():
    """全テストの実行"""
    print("ChainSearchAI GameController統合テスト開始\n")
    
    tests = [
        ("ChainSearchAI GameController統合", test_chain_search_ai_game_integration),
        ("AI比較テスト", test_ai_comparison),
    ]
    
    passed_tests = 0
    failed_tests = 0
    
    for test_name, test_func in tests:
        try:
            if test_func():
                print(f"✅ {test_name}: 成功")
                passed_tests += 1
            else:
                print(f"❌ {test_name}: 失敗")
                failed_tests += 1
        except Exception as e:
            print(f"❌ {test_name}: 例外発生 - {e}")
            import traceback
            traceback.print_exc()
            failed_tests += 1
    
    print(f"\n=== テスト結果 ===")
    print(f"成功: {passed_tests}")
    print(f"失敗: {failed_tests}")
    print(f"成功率: {passed_tests/(passed_tests+failed_tests)*100:.1f}%")
    
    return failed_tests == 0

if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)