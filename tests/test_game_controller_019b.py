#!/usr/bin/env python3
"""
RLPlayerAIとgame_controller.pyの統合テスト
"""
import sys
import os

# プロジェクトルートをパスに追加
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import puyo_ai_platform as pap
from python.ui.game_controller import GameController
from python.ui.player_controller import AIPlayerController

def test_rl_player_ai_game_integration():
    """RLPlayerAIとGameControllerの統合テスト"""
    print("=== RLPlayerAI GameController統合テスト ===")
    
    try:
        # AIマネージャーからRLPlayerAIを作成
        ai_manager = pap.ai.get_global_ai_manager()
        ai_names = ai_manager.get_registered_ai_names()
        print(f"利用可能AI: {ai_names}")
        
        if "rl_player" not in ai_names:
            print("ERROR: RLPlayerAI not available")
            return False
        
        # RLPlayerAIインスタンス作成
        rl_ai = ai_manager.create_ai("rl_player")
        if rl_ai is None:
            print("ERROR: Failed to create RLPlayerAI")
            return False
        
        # AIPlayerController作成
        ai_controller = AIPlayerController(rl_ai, "RLPlayerAI Test")
        print(f"AIPlayerController作成成功: {ai_controller.get_player_name()}")
        
        # GameController作成（UIなしでテスト）
        game_controller = GameController(pap.GameMode.TOKOTON, ai_controller)
        
        # ゲームセットアップ
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
        max_updates = 15  # 15回のアップデートでテスト
        
        # RLPlayerAIの学習的な特徴をテスト
        decisions_made = []
        
        while update_count < max_updates:
            try:
                # ゲーム状態のアップデート
                continue_game = game_controller.update()
                
                if not continue_game:
                    print(f"ゲーム終了が検出されました (アップデート{update_count+1}回目)")
                    break
                
                # AI判定の記録（学習効果テスト用）
                if hasattr(ai_controller, 'last_command') and ai_controller.last_command != 'None':
                    decisions_made.append(ai_controller.last_command)
                
                update_count += 1
                
                if update_count % 5 == 0:
                    print(f"アップデート {update_count}/15 完了")
                
            except Exception as e:
                print(f"ゲームアップデートエラー: {e}")
                import traceback
                traceback.print_exc()
                return False
        
        print(f"✅ {update_count}回のゲームアップデートが正常に完了")
        print(f"AI実行コマンド数: {len(decisions_made)}")
        
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

def test_all_ais_game_integration():
    """3つのAI全てのGameController統合テスト"""
    print("\n=== 全AI GameController統合テスト ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    ai_names = ["random", "chain_search", "rl_player"]
    
    results = {}
    
    for ai_name in ai_names:
        print(f"\n--- {ai_name} AI テスト ---")
        
        try:
            # AI作成
            ai_instance = ai_manager.create_ai(ai_name)
            if ai_instance is None:
                print(f"ERROR: {ai_name} AI作成失敗")
                results[ai_name] = False
                continue
            
            # AIPlayerController作成
            ai_controller = AIPlayerController(ai_instance, f"{ai_name} Test")
            
            # GameController作成
            game_controller = GameController(pap.GameMode.TOKOTON, ai_controller)
            game_controller.setup_game([ai_controller.get_player_name()])
            
            # 短時間のゲームプレイシミュレーション
            successful_updates = 0
            for _ in range(5):
                try:
                    if game_controller.update():
                        successful_updates += 1
                    else:
                        break
                except Exception as e:
                    print(f"{ai_name} アップデートエラー: {e}")
                    break
            
            # 終了処理
            game_controller.quit()
            
            print(f"{ai_name}: {successful_updates}/5 アップデート成功")
            results[ai_name] = successful_updates >= 3  # 3回以上成功すればOK
            
        except Exception as e:
            print(f"{ai_name} 統合テストエラー: {e}")
            results[ai_name] = False
    
    # 結果集計
    successful_ais = sum(1 for success in results.values() if success)
    total_ais = len(results)
    
    print(f"\n統合テスト結果:")
    for ai_name, success in results.items():
        status = "✅ 成功" if success else "❌ 失敗"
        print(f"  {ai_name}: {status}")
    
    print(f"成功AI数: {successful_ais}/{total_ais}")
    
    return successful_ais == total_ais

def test_ai_behavior_comparison():
    """各AIの行動パターン比較"""
    print("\n=== AI行動パターン比較テスト ===")
    
    import time
    
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
    
    # 各AIの行動パターン分析
    ai_configs = [
        ("random", "RandomAI"),
        ("chain_search", "ChainSearchAI"),
        ("rl_player", "RLPlayerAI")
    ]
    
    analysis_results = {}
    
    for ai_key, ai_name in ai_configs:
        print(f"\n{ai_name} 行動分析:")
        
        ai_instance = ai_manager.create_ai(ai_key)
        if ai_instance is None:
            print(f"  {ai_name}: AI作成失敗")
            continue
        
        ai_instance.initialize()
        
        # 10回の判定を実行
        positions = []
        think_times = []
        confidences = []
        
        for i in range(10):
            start_time = time.time()
            decision = ai_instance.think(game_state)
            think_time = time.time() - start_time
            
            positions.append((decision.x, decision.r))
            think_times.append(think_time * 1000)  # ms
            confidences.append(decision.confidence)
        
        # 行動パターン分析
        unique_positions = set(positions)
        avg_think_time = sum(think_times) / len(think_times)
        avg_confidence = sum(confidences) / len(confidences)
        
        most_common_pos = max(unique_positions, key=lambda pos: positions.count(pos))
        most_common_count = positions.count(most_common_pos)
        
        print(f"  ユニーク位置数: {len(unique_positions)}/10")
        print(f"  最頻出位置: ({most_common_pos[0]}, {most_common_pos[1]}) ({most_common_count}回)")
        print(f"  平均思考時間: {avg_think_time:.2f}ms")
        print(f"  平均確信度: {avg_confidence:.3f}")
        
        analysis_results[ai_name] = {
            "diversity": len(unique_positions) / 10.0,
            "avg_think_time": avg_think_time,
            "avg_confidence": avg_confidence,
            "consistency": most_common_count / 10.0
        }
    
    # 比較結果表示
    print(f"\n=== 総合比較 ===")
    print("AI名\t\t多様性\t思考時間\t確信度\t一貫性")
    print("-" * 60)
    for ai_name, metrics in analysis_results.items():
        print(f"{ai_name:15}\t{metrics['diversity']:.2f}\t{metrics['avg_think_time']:6.2f}ms\t{metrics['avg_confidence']:.3f}\t{metrics['consistency']:.2f}")
    
    return True

def run_all_tests():
    """全テストの実行"""
    print("RLPlayerAI GameController統合テスト開始\n")
    
    tests = [
        ("RLPlayerAI GameController統合", test_rl_player_ai_game_integration),
        ("全AI GameController統合", test_all_ais_game_integration),
        ("AI行動パターン比較", test_ai_behavior_comparison),
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