#!/usr/bin/env python3
"""
RLPlayerAI（強化学習型AI）のテスト
"""
import sys
import os

# プロジェクトルートをパスに追加
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import puyo_ai_platform as pap

def test_rl_player_ai_basic():
    """RLPlayerAIの基本機能テスト"""
    print("=== RLPlayerAI基本機能テスト ===")
    
    # グローバルAIマネージャーの取得
    ai_manager = pap.ai.get_global_ai_manager()
    
    # 登録済みAI一覧の取得
    ai_names = ai_manager.get_registered_ai_names()
    print(f"登録済みAI: {ai_names}")
    
    # RLPlayerAIが登録されているか確認
    if "rl_player" not in ai_names:
        print("ERROR: RLPlayerAI not registered")
        return False
    
    # RLPlayerAIの作成
    rl_ai = ai_manager.create_ai("rl_player")
    
    if rl_ai is None:
        print("ERROR: RLPlayerAI作成失敗")
        return False
    
    print(f"RLPlayerAI作成成功: {rl_ai.get_name()}")
    print(f"AI種別: {rl_ai.get_type()}")
    print(f"AIバージョン: {rl_ai.get_version()}")
    
    # 初期化テスト
    init_result = rl_ai.initialize()
    print(f"初期化結果: {init_result}")
    print(f"初期化状態: {rl_ai.is_initialized()}")
    
    if not init_result:
        print("ERROR: RLPlayerAI初期化失敗")
        return False
    
    return True

def test_rl_player_ai_thinking():
    """RLPlayerAI思考機能テスト"""
    print("\n=== RLPlayerAI思考機能テスト ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    rl_ai = ai_manager.create_ai("rl_player")
    
    if not rl_ai.initialize():
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
    for i in range(5):
        decision = rl_ai.think(game_state)
        decisions.append(decision)
        print(f"思考{i+1}: x={decision.x}, r={decision.r}, 確信度={decision.confidence:.3f}")
        print(f"  理由: {decision.reason}")
        print(f"  コマンド数: {len(decision.move_commands)}")
        if len(decision.move_commands) > 0:
            command_str = ' '.join([str(cmd).split('.')[-1] for cmd in decision.move_commands])
            print(f"  コマンドシーケンス: {command_str}")
    
    # 探索的行動の確認（ε-greedyによりランダム要素があるか）
    unique_positions = set((d.x, d.r) for d in decisions)
    print(f"5回の思考で{len(unique_positions)}種類の異なる位置を選択")
    if len(unique_positions) > 1:
        print("✅ 探索的行動を確認（ε-greedy戦略動作中）")
    else:
        print("⚠️  同じ位置のみ選択（活用モードまたは学習不足）")
    
    # 思考時間制限テスト
    think_time = rl_ai.get_think_time_ms()
    print(f"思考時間制限: {think_time}ms")
    
    # デバッグ情報テスト
    debug_info = rl_ai.get_debug_info()
    print(f"デバッグ情報: {debug_info}")
    
    return True

def test_rl_player_ai_parameters():
    """RLPlayerAIパラメータテスト"""
    print("\n=== RLPlayerAIパラメータテスト ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    
    # パラメータ付きでAI作成後に設定
    rl_ai = ai_manager.create_ai("rl_player")
    rl_ai.set_parameter("learning_rate", "0.2")
    rl_ai.set_parameter("epsilon", "0.5")
    rl_ai.set_parameter("discount_factor", "0.8")
    rl_ai.initialize()
    
    # パラメータ確認
    lr = rl_ai.get_parameter("learning_rate")
    eps = rl_ai.get_parameter("epsilon")
    gamma = rl_ai.get_parameter("discount_factor")
    print(f"設定パラメータ - 学習率: {lr}, ε: {eps}, 割引率: {gamma}")
    
    # 思考時間制限の確認
    think_time = rl_ai.get_think_time_ms()
    print(f"思考時間制限: {think_time}ms")
    
    # デバッグ情報でパラメータ反映確認
    debug_info = rl_ai.get_debug_info()
    print(f"デバッグ情報: {debug_info}")
    
    # 複数回思考実行（学習パラメータでの挙動確認）
    gm = pap.GameManager(pap.GameMode.TOKOTON)
    gm.add_player('Test Player', pap.PlayerType.HUMAN)
    player = gm.get_player(0)
    field = player.get_field()
    
    game_state = pap.ai.GameState()
    game_state.player_id = 0
    game_state.turn_count = 1
    game_state.is_versus_mode = False
    game_state.current_pair = pap.PuyoPair(pap.PuyoColor.RED, pap.PuyoColor.GREEN)
    game_state.set_own_field(field)
    
    print("パラメータ調整後の思考テスト:")
    for i in range(3):
        decision = rl_ai.think(game_state)
        print(f"  {i+1}: ({decision.x}, {decision.r}) 確信度={decision.confidence:.3f}")
    
    return True

def test_rl_player_performance():
    """RLPlayerAI性能テスト"""
    print("\n=== RLPlayerAI性能テスト ===")
    
    import time
    
    ai_manager = pap.ai.get_global_ai_manager()
    rl_ai = ai_manager.create_ai("rl_player")
    rl_ai.initialize()
    
    # ゲーム状態セットアップ
    gm = pap.GameManager(pap.GameMode.TOKOTON)
    gm.add_player('Test Player', pap.PlayerType.HUMAN)
    player = gm.get_player(0)
    field = player.get_field()
    
    game_state = pap.ai.GameState()
    game_state.player_id = 0
    game_state.turn_count = 1
    game_state.is_versus_mode = False
    game_state.current_pair = pap.PuyoPair(pap.PuyoColor.YELLOW, pap.PuyoColor.PURPLE)
    game_state.set_own_field(field)
    
    # 思考時間計測
    total_time = 0.0
    num_tests = 5
    
    for i in range(num_tests):
        start_time = time.time()
        decision = rl_ai.think(game_state)
        elapsed_time = time.time() - start_time
        total_time += elapsed_time
        
        print(f"テスト{i+1}: {elapsed_time*1000:.2f}ms - 決定({decision.x}, {decision.r}) 確信度={decision.confidence:.3f}")
    
    avg_time = total_time / num_tests
    print(f"平均思考時間: {avg_time*1000:.2f}ms")
    
    # 思考時間制限内かチェック
    time_limit = rl_ai.get_think_time_ms()
    if avg_time * 1000 <= time_limit:
        print(f"✅ 思考時間制限({time_limit}ms)内で動作")
    else:
        print(f"⚠️ 思考時間制限({time_limit}ms)を超過")
    
    return True

def test_ai_comparison_all():
    """3つのAIの比較テスト"""
    print("\n=== 3つのAI比較テスト ===")
    
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
    ais = {
        "RandomAI": ai_manager.create_ai("random"),
        "ChainSearchAI": ai_manager.create_ai("chain_search"),
        "RLPlayerAI": ai_manager.create_ai("rl_player")
    }
    
    for name, ai in ais.items():
        if ai:
            ai.initialize()
    
    # 各AIの判定を比較
    print("各AIの判定比較:")
    for name, ai in ais.items():
        if ai is None:
            print(f"  {name}: 作成失敗")
            continue
        
        decisions = []
        for _ in range(3):
            decision = ai.think(game_state)
            decisions.append(f"({decision.x},{decision.r})")
        
        print(f"  {name}: {' '.join(decisions)} (type: {ai.get_type()})")
    
    # 思考時間比較
    import time
    print("\n思考時間比較:")
    for name, ai in ais.items():
        if ai is None:
            continue
        
        start_time = time.time()
        ai.think(game_state)
        elapsed_time = time.time() - start_time
        
        print(f"  {name}: {elapsed_time*1000:.2f}ms (制限: {ai.get_think_time_ms()}ms)")
    
    return True

def run_all_tests():
    """全テストの実行"""
    print("RLPlayerAI（チケット019-Stage2）統合テスト開始\n")
    
    tests = [
        ("RLPlayerAI基本機能", test_rl_player_ai_basic),
        ("RLPlayerAI思考機能", test_rl_player_ai_thinking),
        ("RLPlayerAIパラメータ", test_rl_player_ai_parameters),
        ("RLPlayerAI性能", test_rl_player_performance),
        ("3つのAI比較", test_ai_comparison_all),
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