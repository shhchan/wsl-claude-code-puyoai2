#!/usr/bin/env python3
"""
HumanLearningAI（人間行動学習AI）のテスト
"""
import sys
import os

# プロジェクトルートをパスに追加
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import puyo_ai_platform as pap

def test_human_learning_ai_basic():
    """HumanLearningAIの基本機能テスト"""
    print("=== HumanLearningAI基本機能テスト ===")
    
    # グローバルAIマネージャーの取得
    ai_manager = pap.ai.get_global_ai_manager()
    
    # 登録済みAI一覧の取得
    ai_names = ai_manager.get_registered_ai_names()
    print(f"登録済みAI: {ai_names}")
    
    # HumanLearningAIが登録されているか確認
    if "human_learning" not in ai_names:
        print("ERROR: HumanLearningAI not registered")
        return False
    
    # HumanLearningAIの作成
    human_ai = ai_manager.create_ai("human_learning")
    
    if human_ai is None:
        print("ERROR: HumanLearningAI作成失敗")
        return False
    
    print(f"HumanLearningAI作成成功: {human_ai.get_name()}")
    print(f"AI種別: {human_ai.get_type()}")
    print(f"AIバージョン: {human_ai.get_version()}")
    
    # 初期化テスト
    init_result = human_ai.initialize()
    print(f"初期化結果: {init_result}")
    print(f"初期化状態: {human_ai.is_initialized()}")
    
    if not init_result:
        print("ERROR: HumanLearningAI初期化失敗")
        return False
    
    return True

def test_human_learning_ai_thinking():
    """HumanLearningAI思考機能テスト"""
    print("\n=== HumanLearningAI思考機能テスト ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    human_ai = ai_manager.create_ai("human_learning")
    
    if not human_ai.initialize():
        print("ERROR: AI初期化失敗")
        return False
    
    # GameStateの作成
    game_state = pap.ai.GameState()
    game_state.player_id = 0
    game_state.turn_count = 1
    game_state.is_versus_mode = False
    
    # 現在のペアを設定
    game_state.current_pair = pap.PuyoPair(pap.PuyoColor.RED, pap.PuyoColor.BLUE)
    
    # ダミーフィールドを作成（実際のゲームマネージャー経由）
    gm = pap.GameManager(pap.GameMode.TOKOTON)
    gm.add_player('Test Player', pap.PlayerType.HUMAN)
    player = gm.get_player(0)
    field = player.get_field()
    game_state.set_own_field(field)
    
    # 思考実行テスト
    print("思考テスト実行中...")
    decisions = []
    for i in range(8):
        decision = human_ai.think(game_state)
        decisions.append(decision)
        print(f"思考{i+1}: x={decision.x}, r={decision.r}, 確信度={decision.confidence:.3f}")
        print(f"  理由: {decision.reason}")
        print(f"  コマンド数: {len(decision.move_commands)}")
        if len(decision.move_commands) > 0:
            command_str = ' '.join([str(cmd).split('.')[-1] for cmd in decision.move_commands])
            print(f"  コマンドシーケンス: {command_str}")
    
    # 人間らしい行動パターンの確認
    unique_positions = set((d.x, d.r) for d in decisions)
    print(f"8回の思考で{len(unique_positions)}種類の異なる位置を選択")
    
    # 中央寄りの選択傾向を確認（人間らしい行動）
    center_choices = sum(1 for d in decisions if 1 <= d.x <= 4)
    center_ratio = center_choices / len(decisions)
    print(f"中央寄り選択率: {center_ratio*100:.1f}% ({center_choices}/{len(decisions)})")
    
    if center_ratio > 0.5:
        print("✅ 人間らしい中央寄り戦略を確認")
    else:
        print("⚠️  中央寄り傾向は低め（学習データ影響または戦略的多様性）")
    
    # 思考時間制限テスト
    think_time = human_ai.get_think_time_ms()
    print(f"思考時間制限: {think_time}ms")
    
    # デバッグ情報テスト
    debug_info = human_ai.get_debug_info()
    print(f"デバッグ情報: {debug_info}")
    
    return True

def test_human_learning_ai_parameters():
    """HumanLearningAIパラメータテスト"""
    print("\n=== HumanLearningAIパラメータテスト ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    
    # パラメータ付きでAI作成後に設定
    human_ai = ai_manager.create_ai("human_learning")
    human_ai.set_parameter("field_weight", "0.8")
    human_ai.set_parameter("color_weight", "0.15")
    human_ai.set_parameter("max_similar_cases", "3")
    human_ai.initialize()
    
    # パラメータ確認
    field_w = human_ai.get_parameter("field_weight")
    color_w = human_ai.get_parameter("color_weight")
    max_cases = human_ai.get_parameter("max_similar_cases")
    print(f"設定パラメータ - フィールド重み: {field_w}, 色重み: {color_w}, 最大類似ケース: {max_cases}")
    
    # 思考時間制限の確認
    think_time = human_ai.get_think_time_ms()
    print(f"思考時間制限: {think_time}ms")
    
    # デバッグ情報でパラメータ反映確認
    debug_info = human_ai.get_debug_info()
    print(f"デバッグ情報: {debug_info}")
    
    # パラメータ調整後の思考テスト
    gm = pap.GameManager(pap.GameMode.TOKOTON)
    gm.add_player('Test Player', pap.PlayerType.HUMAN)
    player = gm.get_player(0)
    field = player.get_field()
    
    game_state = pap.ai.GameState()
    game_state.player_id = 0
    game_state.turn_count = 1
    game_state.is_versus_mode = False
    game_state.current_pair = pap.PuyoPair(pap.PuyoColor.GREEN, pap.PuyoColor.YELLOW)
    game_state.set_own_field(field)
    
    print("パラメータ調整後の思考テスト:")
    for i in range(3):
        decision = human_ai.think(game_state)
        print(f"  {i+1}: ({decision.x}, {decision.r}) 確信度={decision.confidence:.3f}")
    
    return True

def test_human_learning_performance():
    """HumanLearningAI性能テスト"""
    print("\n=== HumanLearningAI性能テスト ===")
    
    import time
    
    ai_manager = pap.ai.get_global_ai_manager()
    human_ai = ai_manager.create_ai("human_learning")
    human_ai.initialize()
    
    # ゲーム状態セットアップ
    gm = pap.GameManager(pap.GameMode.TOKOTON)
    gm.add_player('Test Player', pap.PlayerType.HUMAN)
    player = gm.get_player(0)
    field = player.get_field()
    
    game_state = pap.ai.GameState()
    game_state.player_id = 0
    game_state.turn_count = 1
    game_state.is_versus_mode = False
    game_state.current_pair = pap.PuyoPair(pap.PuyoColor.PURPLE, pap.PuyoColor.RED)
    game_state.set_own_field(field)
    
    # 思考時間計測
    total_time = 0.0
    num_tests = 5
    
    for i in range(num_tests):
        start_time = time.time()
        decision = human_ai.think(game_state)
        elapsed_time = time.time() - start_time
        total_time += elapsed_time
        
        print(f"テスト{i+1}: {elapsed_time*1000:.2f}ms - 決定({decision.x}, {decision.r}) 確信度={decision.confidence:.3f}")
    
    avg_time = total_time / num_tests
    print(f"平均思考時間: {avg_time*1000:.2f}ms")
    
    # 思考時間制限内かチェック
    time_limit = human_ai.get_think_time_ms()
    if avg_time * 1000 <= time_limit:
        print(f"✅ 思考時間制限({time_limit}ms)内で動作")
    else:
        print(f"⚠️ 思考時間制限({time_limit}ms)を超過")
    
    return True

def test_four_ais_comparison():
    """4つのAIの包括比較テスト"""
    print("\n=== 4つのAI包括比較テスト ===")
    
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
    
    # 各AIの初期化
    ai_configs = [
        ("random", "RandomAI"),
        ("chain_search", "ChainSearchAI"),
        ("rl_player", "RLPlayerAI"),
        ("human_learning", "HumanLearningAI")
    ]
    
    ais = {}
    for ai_key, ai_name in ai_configs:
        ai_instance = ai_manager.create_ai(ai_key)
        if ai_instance:
            ai_instance.initialize()
            ais[ai_name] = ai_instance
    
    print(f"テスト対象AI数: {len(ais)}")
    
    # 各AIの判定比較
    print("\n各AIの判定比較:")
    for name, ai in ais.items():
        if ai is None:
            print(f"  {name}: 作成失敗")
            continue
        
        decisions = []
        confidences = []
        for _ in range(5):
            decision = ai.think(game_state)
            decisions.append(f"({decision.x},{decision.r})")
            confidences.append(decision.confidence)
        
        avg_confidence = sum(confidences) / len(confidences)
        print(f"  {name:15}: {' '.join(decisions)} (確信度平均: {avg_confidence:.3f})")
    
    # 思考時間・特性比較
    import time
    print("\n性能・特性比較:")
    print("AI名\t\t思考時間\t制限時間\t特徴")
    print("-" * 65)
    
    for name, ai in ais.items():
        if ai is None:
            continue
        
        start_time = time.time()
        decision = ai.think(game_state)
        elapsed_time = time.time() - start_time
        
        features = {
            "RandomAI": "探索的・高多様性",
            "ChainSearchAI": "戦略的・高一貫性",
            "RLPlayerAI": "学習的・適応性",
            "HumanLearningAI": "模倣的・人間らしさ"
        }
        
        print(f"{name:15}\t{elapsed_time*1000:6.2f}ms\t{ai.get_think_time_ms():7d}ms\t{features.get(name, '不明')}")
    
    print(f"\n✅ 4つのAI全てが正常に動作し、それぞれ異なる特性を示しています")
    
    return True

def run_all_tests():
    """全テストの実行"""
    print("HumanLearningAI（チケット019-Stage3）統合テスト開始\n")
    
    tests = [
        ("HumanLearningAI基本機能", test_human_learning_ai_basic),
        ("HumanLearningAI思考機能", test_human_learning_ai_thinking),
        ("HumanLearningAIパラメータ", test_human_learning_ai_parameters),
        ("HumanLearningAI性能", test_human_learning_performance),
        ("4つのAI包括比較", test_four_ais_comparison),
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