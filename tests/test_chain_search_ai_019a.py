#!/usr/bin/env python3
"""
ChainSearchAI（連鎖探索型AI）のテスト
"""
import sys
import os

# プロジェクトルートをパスに追加
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import puyo_ai_platform as pap

def test_chain_search_ai_basic():
    """ChainSearchAIの基本機能テスト"""
    print("=== ChainSearchAI基本機能テスト ===")
    
    # グローバルAIマネージャーの取得
    ai_manager = pap.ai.get_global_ai_manager()
    
    # 登録済みAI一覧の取得
    ai_names = ai_manager.get_registered_ai_names()
    print(f"登録済みAI: {ai_names}")
    
    # ChainSearchAIが登録されているか確認
    if "chain_search" not in ai_names:
        print("ERROR: ChainSearchAI not registered")
        return False
    
    # ChainSearchAIの作成
    chain_ai = ai_manager.create_ai("chain_search")
    
    if chain_ai is None:
        print("ERROR: ChainSearchAI作成失敗")
        return False
    
    print(f"ChainSearchAI作成成功: {chain_ai.get_name()}")
    print(f"AI種別: {chain_ai.get_type()}")
    print(f"AIバージョン: {chain_ai.get_version()}")
    
    # 初期化テスト
    init_result = chain_ai.initialize()
    print(f"初期化結果: {init_result}")
    print(f"初期化状態: {chain_ai.is_initialized()}")
    
    if not init_result:
        print("ERROR: ChainSearchAI初期化失敗")
        return False
    
    return True

def test_chain_search_ai_thinking():
    """ChainSearchAI思考機能テスト"""
    print("\n=== ChainSearchAI思考機能テスト ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    chain_ai = ai_manager.create_ai("chain_search")
    
    if not chain_ai.initialize():
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
    for i in range(3):
        decision = chain_ai.think(game_state)
        print(f"思考{i+1}: x={decision.x}, r={decision.r}, 確信度={decision.confidence:.2f}")
        print(f"  理由: {decision.reason}")
        print(f"  コマンド数: {len(decision.move_commands)}")
        if len(decision.move_commands) > 0:
            command_str = ' '.join([str(cmd).split('.')[-1] for cmd in decision.move_commands])
            print(f"  コマンドシーケンス: {command_str}")
    
    # 思考時間制限テスト
    think_time = chain_ai.get_think_time_ms()
    print(f"思考時間制限: {think_time}ms")
    
    # デバッグ情報テスト
    debug_info = chain_ai.get_debug_info()
    print(f"デバッグ情報: {debug_info}")
    
    return True

def test_chain_search_ai_parameters():
    """ChainSearchAIパラメータテスト"""
    print("\n=== ChainSearchAIパラメータテスト ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    
    # パラメータ付きでAI作成
    params = {
        "search_depth": "4",
        "chain_weight": "15.0",
        "stability_weight": "2.0",
        "trigger_bonus": "100.0"
    }
    
    # パラメータは直接設定できないため、作成後に設定
    chain_ai = ai_manager.create_ai("chain_search")
    chain_ai.set_parameter("search_depth", "4")
    chain_ai.set_parameter("chain_weight", "15.0")
    chain_ai.initialize()
    
    # パラメータ確認
    depth = chain_ai.get_parameter("search_depth")
    weight = chain_ai.get_parameter("chain_weight")
    print(f"設定パラメータ - 探索深度: {depth}, 連鎖重み: {weight}")
    
    # 思考時間制限の変化確認
    think_time = chain_ai.get_think_time_ms()
    print(f"パラメータ設定後の思考時間制限: {think_time}ms")
    
    # デバッグ情報でパラメータ反映確認
    debug_info = chain_ai.get_debug_info()
    print(f"デバッグ情報: {debug_info}")
    
    return True

def test_chain_search_performance():
    """ChainSearchAI性能テスト"""
    print("\n=== ChainSearchAI性能テスト ===")
    
    import time
    
    ai_manager = pap.ai.get_global_ai_manager()
    chain_ai = ai_manager.create_ai("chain_search")
    chain_ai.initialize()
    
    # ゲーム状態セットアップ
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
    
    # 思考時間計測
    total_time = 0.0
    num_tests = 5
    
    for i in range(num_tests):
        start_time = time.time()
        decision = chain_ai.think(game_state)
        elapsed_time = time.time() - start_time
        total_time += elapsed_time
        
        print(f"テスト{i+1}: {elapsed_time*1000:.1f}ms - 決定({decision.x}, {decision.r})")
    
    avg_time = total_time / num_tests
    print(f"平均思考時間: {avg_time*1000:.1f}ms")
    
    # 思考時間制限内かチェック
    time_limit = chain_ai.get_think_time_ms()
    if avg_time * 1000 <= time_limit:
        print(f"✅ 思考時間制限({time_limit}ms)内で動作")
    else:
        print(f"⚠️ 思考時間制限({time_limit}ms)を超過")
    
    return True

def run_all_tests():
    """全テストの実行"""
    print("ChainSearchAI（チケット019-Stage1）統合テスト開始\n")
    
    tests = [
        ("ChainSearchAI基本機能", test_chain_search_ai_basic),
        ("ChainSearchAI思考機能", test_chain_search_ai_thinking),
        ("ChainSearchAIパラメータ", test_chain_search_ai_parameters),
        ("ChainSearchAI性能", test_chain_search_performance),
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