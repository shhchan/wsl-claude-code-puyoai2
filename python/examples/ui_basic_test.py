#!/usr/bin/env python3
"""
UI基本機能テスト
描画システムの動作確認
"""

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

import puyo_ai_platform as pap
from python.ui.renderer import GameVisualizer
import time

def test_basic_rendering():
    """基本描画テスト"""
    print("=== UI基本描画テスト ===")
    
    # ゲームマネージャー作成
    game = pap.GameManager(pap.GameMode.TOKOTON)
    game.add_player("Test Player", pap.PlayerType.HUMAN)
    
    # 可視化システム作成
    visualizer = GameVisualizer()
    
    # テスト用フィールドを設定
    player = game.get_player(0)
    field = player.get_field()
    
    # いくつかのぷよを配置
    field.set_puyo(pap.Position(0, 0), pap.PuyoColor.RED)
    field.set_puyo(pap.Position(1, 0), pap.PuyoColor.GREEN)
    field.set_puyo(pap.Position(2, 0), pap.PuyoColor.BLUE)
    field.set_puyo(pap.Position(3, 0), pap.PuyoColor.YELLOW)
    field.set_puyo(pap.Position(0, 1), pap.PuyoColor.PURPLE)
    field.set_puyo(pap.Position(1, 1), pap.PuyoColor.RED)
    
    # テスト用ぷよペア
    test_pair = pap.PuyoPair(
        axis=pap.PuyoColor.RED,
        child=pap.PuyoColor.BLUE,
        pos=pap.Position(2, 5),
        rot=pap.Rotation.UP
    )
    
    print("基本描画テストを開始します...")
    print("ESCキーで終了してください")
    
    # 描画ループ
    running = True
    while running and visualizer.is_running():
        # イベント処理
        key = visualizer.handle_events()
        if key == 27:  # ESC
            running = False
        
        # 描画
        visualizer.render_game(game, test_pair, True)
        time.sleep(0.016)  # 約60FPS
    
    visualizer.quit()
    print("基本描画テスト完了")

if __name__ == "__main__":
    print("UI基本機能テスト")
    print("=" * 50)
    
    try:
        test_basic_rendering()
        print("\nテスト完了")
        
    except Exception as e:
        print(f"\nエラーが発生しました: {e}")
        import traceback
        traceback.print_exc()