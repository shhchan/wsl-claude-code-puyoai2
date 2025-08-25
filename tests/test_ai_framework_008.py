#!/usr/bin/env python3
"""
AIフレームワーク（チケット008）のテスト
"""
import sys
import os

# プロジェクトルートをパスに追加
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import puyo_ai_platform as pap

def test_ai_manager_basic():
    """AIマネージャーの基本機能テスト"""
    print("=== AIマネージャー基本機能テスト ===")
    
    # グローバルAIマネージャーの取得
    ai_manager = pap.ai.get_global_ai_manager()
    
    print(f"AIマネージャー取得成功: {ai_manager is not None}")
    
    # 登録済みAI一覧の取得
    ai_names = ai_manager.get_registered_ai_names()
    print(f"登録済みAI数: {len(ai_names)}")
    print(f"登録済みAI: {ai_names}")
    
    # AIマネージャーの状態表示
    status = ai_manager.get_status()
    print("AIマネージャー状態:")
    print(status)
    
    return True

def test_random_ai_creation():
    """RandomAIの作成テスト"""
    print("\n=== RandomAI作成テスト ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    
    # RandomAIの作成（パラメータなし）
    random_ai = ai_manager.create_ai("random")
    
    if random_ai is None:
        print("ERROR: RandomAI作成失敗")
        return False
    
    print(f"RandomAI作成成功: {random_ai.get_name()}")
    print(f"AI種別: {random_ai.get_type()}")
    print(f"AIバージョン: {random_ai.get_version()}")
    
    # 初期化テスト
    init_result = random_ai.initialize()
    print(f"初期化結果: {init_result}")
    print(f"初期化状態: {random_ai.is_initialized()}")
    
    return True

def test_ai_parameters():
    """AIパラメータ機能テスト"""
    print("\n=== AIパラメータテスト ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    
    # パラメータなしでRandomAIを作成（AIParametersの使用を回避）
    random_ai = ai_manager.create_ai("random")
    
    if random_ai is None:
        print("ERROR: RandomAI作成失敗")
        return False
    
    # パラメータ設定・取得のテスト
    random_ai.set_parameter("test_param", "test_value")
    test_value = random_ai.get_parameter("test_param", "default")
    print(f"test_param パラメータ: {test_value}")
    
    nonexistent_param = random_ai.get_parameter("nonexistent", "default_val")
    print(f"存在しないパラメータ: {nonexistent_param}")
    
    # 全パラメータの取得
    all_params = random_ai.get_all_parameters()
    print(f"全パラメータ数: {len(all_params.keys())}")
    
    # 追加パラメータ設定
    random_ai.set_parameter("seed", "12345")
    seed_value = random_ai.get_parameter("seed")
    print(f"seed パラメータ: {seed_value}")
    
    return True

def test_ai_thinking():
    """AI思考機能テスト"""
    print("\n=== AI思考機能テスト ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    random_ai = ai_manager.create_ai("random")
    
    if not random_ai.initialize():
        print("ERROR: AI初期化失敗")
        return False
    
    # GameStateの作成
    game_state = pap.ai.GameState()
    game_state.player_id = 0
    game_state.turn_count = 1
    game_state.is_versus_mode = False
    
    # 現在のペアを設定
    game_state.current_pair = pap.PuyoPair(pap.PuyoColor.RED, pap.PuyoColor.BLUE)
    
    # 思考実行
    for i in range(5):
        decision = random_ai.think(game_state)
        print(f"思考{i+1}: コマンド={decision.command}, 確信度={decision.confidence:.2f}, 理由={decision.reason}")
    
    return True

def test_ai_info():
    """AI情報取得テスト"""
    print("\n=== AI情報取得テスト ===")
    
    ai_manager = pap.ai.get_global_ai_manager()
    
    try:
        ai_info = ai_manager.get_ai_info("random")
        print(f"AI名: {ai_info.name}")
        print(f"AI種別: {ai_info.type}")
        print(f"AIバージョン: {ai_info.version}")
        print(f"AI説明: {ai_info.description}")
    except Exception as e:
        print(f"ERROR: AI情報取得失敗 - {e}")
        return False
    
    # 存在しないAIの情報取得
    try:
        nonexistent_info = ai_manager.get_ai_info("nonexistent")
        print("ERROR: 存在しないAIの情報が取得できてしまいました")
        return False
    except Exception:
        print("存在しないAIの例外処理: 正常")
    
    return True

def test_game_integration():
    """ゲーム統合テスト（基本的な連携確認）"""
    print("\n=== ゲーム統合テスト ===")
    
    # ゲームマネージャーの作成
    gm = pap.GameManager(pap.GameMode.TOKOTON)
    gm.add_player('AI Test Player', pap.PlayerType.HUMAN)  # 現在はHUMANタイプのみ対応
    
    player = gm.get_player(0)
    if player is None:
        print("ERROR: プレイヤー作成失敗")
        return False
    
    # フィールド情報をGameStateに設定する例
    ai_manager = pap.ai.get_global_ai_manager()
    random_ai = ai_manager.create_ai("random")
    random_ai.initialize()
    
    game_state = pap.ai.GameState()
    game_state.player_id = player.get_id()
    game_state.turn_count = 1
    game_state.is_versus_mode = False
    
    # 現在のフィールドをGameStateに設定
    # Note: フィールドへの参照設定は現在のバインディングでは制限があります
    
    # AI思考実行
    decision = random_ai.think(game_state)
    print(f"ゲーム統合テスト - AI判断: {decision.command}")
    
    print("ゲーム統合テスト完了")
    return True

def run_all_tests():
    """全テストの実行"""
    print("AIフレームワーク（チケット008）統合テスト開始\n")
    
    tests = [
        ("AIマネージャー基本機能", test_ai_manager_basic),
        ("RandomAI作成", test_random_ai_creation),
        ("AIパラメータ", test_ai_parameters),
        ("AI思考機能", test_ai_thinking),
        ("AI情報取得", test_ai_info),
        ("ゲーム統合", test_game_integration),
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
            failed_tests += 1
    
    print(f"\n=== テスト結果 ===")
    print(f"成功: {passed_tests}")
    print(f"失敗: {failed_tests}")
    print(f"成功率: {passed_tests/(passed_tests+failed_tests)*100:.1f}%")
    
    return failed_tests == 0

if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)