#!/usr/bin/env python3
"""
基本的なゲーム動作確認用デモスクリプト
Pythonバインディングの動作テスト
"""

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

import puyo_ai_platform as pap

def test_field_basic_operations():
    """フィールドの基本操作テスト"""
    print("=== フィールド基本操作テスト ===")
    
    # フィールド作成
    field = pap.Field()
    print("空のフィールドを作成しました")
    print(field.to_string())
    
    # ぷよを設置
    pos = pap.Position(2, 0)  # 3列目, 1段目
    field.set_puyo(pos, pap.PuyoColor.RED)
    print(f"\n位置 ({pos.x}, {pos.y}) に赤ぷよを設置:")
    print(field.to_string())
    
    # ぷよを取得
    color = field.get_puyo(pos)
    print(f"位置 ({pos.x}, {pos.y}) のぷよの色: {color}")
    
    # エラーハンドリングテスト
    try:
        invalid_pos = pap.Position(-1, 0)  # 無効な位置
        field.get_puyo(invalid_pos)
    except Exception as e:
        print(f"期待されたエラー: {e}")

def test_puyo_pair_operations():
    """ぷよペア操作テスト"""
    print("\n=== ぷよペア操作テスト ===")
    
    field = pap.Field()
    
    # ぷよペア作成
    pair = pap.PuyoPair(
        axis=pap.PuyoColor.RED, 
        child=pap.PuyoColor.BLUE,
        pos=pap.Position(2, 10),  # 3列目, 11段目
        rot=pap.Rotation.UP
    )
    
    print(f"ぷよペア作成: 軸={pair.axis}, 子={pair.child}")
    print(f"軸位置: ({pair.pos.x}, {pair.pos.y})")
    child_pos = pair.get_child_position()
    print(f"子位置: ({child_pos.x}, {child_pos.y})")
    
    # 設置可能性チェック
    can_place = field.can_place_puyo_pair(pair)
    print(f"設置可能: {can_place}")
    
    if can_place:
        # 設置実行
        success = field.place_puyo_pair(pair)
        print(f"設置結果: {success}")
        print("設置後のフィールド:")
        print(field.to_string())

def test_game_manager_tokoton():
    """とことんモードテスト"""
    print("\n=== とことんモードテスト ===")
    
    # ゲームマネージャー作成
    game = pap.GameManager(pap.GameMode.TOKOTON)
    
    # プレイヤー追加
    game.add_player("Player1", pap.PlayerType.HUMAN)
    print("プレイヤーを追加しました")
    
    # プレイヤー取得
    player = game.get_player(0)
    if player:
        print(f"プレイヤー名: {player.get_name()}")
        print(f"プレイヤータイプ: {player.get_type()}")
        
        # プレイヤー統計を取得
        stats = player.get_stats()
        print(f"統計: スコア={stats.total_score}, 最大連鎖={stats.max_chain}")
    
    # ゲーム状態確認
    print(f"ゲームモード: {game.get_mode()}")
    print(f"ゲーム状態: {game.get_state()}")
    print(f"現在のステップ: {game.get_current_step()}")

def test_game_manager_versus():
    """対戦モードテスト"""
    print("\n=== 対戦モードテスト ===")
    
    # 対戦ゲームマネージャー作成
    game = pap.GameManager(pap.GameMode.VERSUS)
    
    # プレイヤー2人追加
    game.add_player("Player1", pap.PlayerType.HUMAN)
    game.add_player("Player2", pap.PlayerType.HUMAN)
    print("2人のプレイヤーを追加しました")
    
    # 各プレイヤー情報表示
    for i in range(2):
        player = game.get_player(i)
        if player:
            print(f"プレイヤー{i+1}: {player.get_name()}")
            stats = player.get_stats()
            print(f"  スコア: {stats.total_score}")
            print(f"  最大連鎖: {stats.max_chain}")

def test_copy_operations():
    """コピー操作テスト"""
    print("\n=== コピー操作テスト ===")
    
    # 元のフィールド作成
    original = pap.Field()
    original.set_puyo(pap.Position(0, 0), pap.PuyoColor.RED)
    original.set_puyo(pap.Position(1, 0), pap.PuyoColor.BLUE)
    
    print("元のフィールド:")
    print(original.to_string())
    
    # コピー作成
    import copy
    copied = copy.copy(original)
    
    # コピーを変更
    copied.set_puyo(pap.Position(2, 0), pap.PuyoColor.GREEN)
    
    print("\nコピー後のフィールド:")
    print(copied.to_string())
    
    print("\n元のフィールド（変更されていないことを確認）:")
    print(original.to_string())

def main():
    """メイン実行関数"""
    print("Python連携動作確認デモ")
    print("=" * 50)
    
    try:
        # 基本操作テスト
        test_field_basic_operations()
        
        # ぷよペア操作テスト
        test_puyo_pair_operations()
        
        # とことんモードテスト
        test_game_manager_tokoton()
        
        # 対戦モードテスト
        test_game_manager_versus()
        
        # コピー操作テスト
        test_copy_operations()
        
        print("\n" + "=" * 50)
        print("すべてのテストが正常に完了しました！")
        
    except Exception as e:
        print(f"\nエラーが発生しました: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())