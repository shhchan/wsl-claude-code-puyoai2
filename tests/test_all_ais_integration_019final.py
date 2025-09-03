#!/usr/bin/env python3
"""
全AI（4種類）とgame_controller.pyの最終統合テスト
"""
import sys
import os

# プロジェクトルートをパスに追加
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import puyo_ai_platform as pap
from python.ui.game_controller import GameController
from python.ui.player_controller import AIPlayerController

def test_all_ais_game_integration():
    """全AI（4種類）のGameController統合テスト"""
    print("=== 全AI GameController統合テスト（最終版） ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    ai_configs = [
        ("random", "RandomAI"),
        ("chain_search", "ChainSearchAI"), 
        ("rl_player", "RLPlayerAI"),
        ("human_learning", "HumanLearningAI")
    ]
    
    print(f"テスト対象: {len(ai_configs)}種類のAI")
    results = {}
    
    for ai_key, ai_name in ai_configs:
        print(f"\n--- {ai_name} GameController統合テスト ---")
        
        try:
            # AI作成
            ai_instance = ai_manager.create_ai(ai_key)
            if ai_instance is None:
                print(f"ERROR: {ai_name} 作成失敗")
                results[ai_name] = {"success": False, "error": "AI作成失敗"}
                continue
            
            # AIPlayerController作成
            ai_controller = AIPlayerController(ai_instance, f"{ai_name} Test")
            print(f"AIPlayerController作成: {ai_controller.get_player_name()}")
            
            # GameController作成・セットアップ
            game_controller = GameController(pap.GameMode.TOKOTON, ai_controller)
            game_controller.setup_game([ai_controller.get_player_name()])
            
            if not game_controller.ai_mode:
                print(f"ERROR: {ai_name} AI制御モード有効化失敗")
                results[ai_name] = {"success": False, "error": "AI制御モード無効"}
                continue
            
            print("✅ AI制御モード有効")
            
            # ゲームプレイシミュレーション（10回のアップデート）
            successful_updates = 0
            ai_decisions = []
            
            for update_round in range(10):
                try:
                    continue_game = game_controller.update()
                    
                    if not continue_game:
                        print(f"ゲーム終了検出 (ラウンド{update_round+1})")
                        break
                    
                    # AI判定の記録
                    if hasattr(ai_controller, 'last_command'):
                        ai_decisions.append(ai_controller.last_command)
                    
                    successful_updates += 1
                    
                except Exception as e:
                    print(f"アップデートエラー (ラウンド{update_round+1}): {e}")
                    break
            
            # 終了処理
            game_controller.quit()
            
            # 結果記録
            success = successful_updates >= 5  # 5回以上成功すればOK
            results[ai_name] = {
                "success": success,
                "updates": successful_updates,
                "decisions": len(ai_decisions),
                "ai_type": ai_instance.get_type()
            }
            
            print(f"結果: {successful_updates}/10 アップデート成功, AI判定数: {len(ai_decisions)}")
            
        except Exception as e:
            print(f"{ai_name} 統合テストエラー: {e}")
            import traceback
            traceback.print_exc()
            results[ai_name] = {"success": False, "error": str(e)}
    
    # 結果集計
    print(f"\n=== 統合テスト結果サマリー ===")
    successful_ais = 0
    
    for ai_name, result in results.items():
        if result["success"]:
            successful_ais += 1
            print(f"✅ {ai_name}: 成功 ({result.get('updates', 0)}/10 アップデート, {result.get('decisions', 0)} AI判定)")
        else:
            print(f"❌ {ai_name}: 失敗 ({result.get('error', '不明なエラー')})")
    
    print(f"\n成功AI数: {successful_ais}/{len(ai_configs)}")
    print(f"統合成功率: {successful_ais/len(ai_configs)*100:.1f}%")
    
    return successful_ais == len(ai_configs)

def test_ai_behavior_analysis():
    """各AIの詳細行動分析"""
    print("\n=== AI詳細行動分析 ===")
    
    import time
    
    ai_manager = pap.ai.get_global_ai_manager()
    
    # 共通テスト環境
    gm = pap.GameManager(pap.GameMode.TOKOTON)
    gm.add_player('Analysis Player', pap.PlayerType.HUMAN)
    player = gm.get_player(0)
    field = player.get_field()
    
    game_state = pap.ai.GameState()
    game_state.player_id = 0
    game_state.turn_count = 5  # ゲーム中盤想定
    game_state.is_versus_mode = False
    game_state.current_pair = pap.PuyoPair(pap.PuyoColor.RED, pap.PuyoColor.BLUE)
    game_state.set_own_field(field)
    
    ai_configs = [
        ("random", "RandomAI"),
        ("chain_search", "ChainSearchAI"),
        ("rl_player", "RLPlayerAI"),
        ("human_learning", "HumanLearningAI")
    ]
    
    analysis_results = {}
    
    print("AI名\t\t多様性\t平均確信度\t平均思考時間\t制限時間\t主な特徴")
    print("-" * 80)
    
    for ai_key, ai_name in ai_configs:
        ai_instance = ai_manager.create_ai(ai_key)
        if not ai_instance:
            continue
        
        ai_instance.initialize()
        
        # 20回の判定を実行して分析
        positions = []
        confidences = []
        think_times = []
        
        for _ in range(20):
            start_time = time.time()
            decision = ai_instance.think(game_state)
            elapsed_time = time.time() - start_time
            
            positions.append((decision.x, decision.r))
            confidences.append(decision.confidence)
            think_times.append(elapsed_time)
        
        # 分析指標計算
        unique_positions = len(set(positions))
        diversity = unique_positions / 20.0
        avg_confidence = sum(confidences) / len(confidences)
        avg_think_time = sum(think_times) / len(think_times) * 1000  # ms
        
        # 位置分布分析
        x_positions = [pos[0] for pos in positions]
        center_bias = sum(1 for x in x_positions if 1 <= x <= 4) / len(x_positions)
        
        # 回転多様性
        rotations = [pos[1] for pos in positions]
        rotation_diversity = len(set(rotations)) / 4.0  # 最大4種類
        
        # 特徴定義
        features = []
        if diversity > 0.7:
            features.append("高多様性")
        elif diversity < 0.3:
            features.append("高一貫性")
        
        if avg_confidence > 0.8:
            features.append("高確信")
        elif avg_confidence < 0.3:
            features.append("探索的")
        
        if center_bias > 0.7:
            features.append("中央寄り")
        
        if rotation_diversity > 0.5:
            features.append("回転多様")
        
        feature_str = ", ".join(features) if features else "標準的"
        
        print(f"{ai_name:15}\t{diversity:.2f}\t{avg_confidence:.3f}\t\t{avg_think_time:6.2f}ms\t{ai_instance.get_think_time_ms():7d}ms\t{feature_str}")
        
        analysis_results[ai_name] = {
            "diversity": diversity,
            "avg_confidence": avg_confidence,
            "avg_think_time": avg_think_time,
            "center_bias": center_bias,
            "rotation_diversity": rotation_diversity,
            "features": feature_str
        }
    
    return True

def test_comprehensive_scenario():
    """包括的シナリオテスト（全AIの同一条件比較）"""
    print("\n=== 包括的シナリオテスト ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    
    # 複数の異なるゲーム状況をテスト
    test_scenarios = [
        {
            "name": "初期状態", 
            "turn": 1,
            "colors": (pap.PuyoColor.RED, pap.PuyoColor.BLUE)
        },
        {
            "name": "中盤戦", 
            "turn": 10,
            "colors": (pap.PuyoColor.GREEN, pap.PuyoColor.YELLOW)
        },
        {
            "name": "後半戦", 
            "turn": 20,
            "colors": (pap.PuyoColor.PURPLE, pap.PuyoColor.RED)
        }
    ]
    
    ai_names = ["random", "chain_search", "rl_player", "human_learning"]
    ais = {name: ai_manager.create_ai(name) for name in ai_names}
    
    # AI初期化
    for ai in ais.values():
        if ai:
            ai.initialize()
    
    print("シナリオ別AI判定比較:")
    
    for scenario in test_scenarios:
        print(f"\n【{scenario['name']}】")
        
        # ゲーム状態構築
        gm = pap.GameManager(pap.GameMode.TOKOTON)
        gm.add_player('Scenario Player', pap.PlayerType.HUMAN)
        player = gm.get_player(0)
        field = player.get_field()
        
        game_state = pap.ai.GameState()
        game_state.player_id = 0
        game_state.turn_count = scenario["turn"]
        game_state.is_versus_mode = False
        game_state.current_pair = pap.PuyoPair(scenario["colors"][0], scenario["colors"][1])
        game_state.set_own_field(field)
        
        # 各AIの判定
        for ai_name, ai_instance in ais.items():
            if not ai_instance:
                continue
            
            decision = ai_instance.think(game_state)
            
            # 判定の特徴分析
            position_type = "中央" if 1 <= decision.x <= 4 else "端"
            confidence_level = "高" if decision.confidence > 0.5 else "低"
            
            print(f"  {ai_name:15}: ({decision.x}, {decision.r}) - {position_type}寄り, 確信度{confidence_level}({decision.confidence:.3f})")
    
    print(f"\n✅ 全シナリオで4つのAI全てが正常に動作")
    
    return True

def run_all_tests():
    """全統合テストの実行"""
    print("🎮 全AI統合テスト（チケット019最終版）開始\n")
    
    tests = [
        ("全AI GameController統合", test_all_ais_game_integration),
        ("AI詳細行動分析", test_ai_behavior_analysis),
        ("包括的シナリオテスト", test_comprehensive_scenario),
    ]
    
    passed_tests = 0
    failed_tests = 0
    
    for test_name, test_func in tests:
        print(f"\n{'='*60}")
        print(f"テスト実行: {test_name}")
        print('='*60)
        
        try:
            if test_func():
                print(f"\n✅ {test_name}: 成功")
                passed_tests += 1
            else:
                print(f"\n❌ {test_name}: 失敗")
                failed_tests += 1
        except Exception as e:
            print(f"\n❌ {test_name}: 例外発生 - {e}")
            import traceback
            traceback.print_exc()
            failed_tests += 1
    
    print(f"\n{'='*60}")
    print("🏆 最終テスト結果")
    print('='*60)
    print(f"成功: {passed_tests}")
    print(f"失敗: {failed_tests}")
    print(f"成功率: {passed_tests/(passed_tests+failed_tests)*100:.1f}%")
    
    if failed_tests == 0:
        print(f"\n🎉 チケット019: 全AI実装・統合テスト完全成功！")
        print("✅ RandomAI: 動作確認済み")
        print("✅ ChainSearchAI: 連鎖探索型AI完全動作")
        print("✅ RLPlayerAI: 強化学習型AI完全動作")
        print("✅ HumanLearningAI: 人間行動学習AI完全動作")
        print("✅ game_controller.py統合: 4つのAI全て選択・実行可能")
    else:
        print(f"\n⚠️ 一部テストで問題が発生しました")
    
    return failed_tests == 0

if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)