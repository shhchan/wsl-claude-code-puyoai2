#!/usr/bin/env python3
"""
インタラクティブUIデモ
実際のゲームプレイテスト
"""

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from python.ui.game_controller import GameController
import puyo_ai_platform as pap

def main():
    """メインデモ実行"""
    print("Puyo Puyo Interactive UI Demo")
    print("=" * 50)
    print()
    
    print("操作方法:")
    print("  A/D     - 左右移動")
    print("  ↓/→     - 回転（反時計回り/時計回り）")  
    print("  W       - ドロップ")
    print("  R       - ゲームリセット")
    print("  ESC     - 終了")
    print()
    print("ゲームを開始します...")
    
    try:
        # ゲームコントローラー初期化
        controller = GameController(pap.GameMode.TOKOTON)
        controller.setup_game(["Human Player"])
        
        print("ゲームウィンドウが開きました。")
        print("ウィンドウを閉じるか、ESCキーで終了してください。")
        
        # メインゲームループ
        running = True
        while running and controller.is_running():
            # イベント処理
            running = controller.handle_events()
            
            if running:
                # ゲーム状態更新
                running = controller.update()
                
                # 描画
                controller.render()
        
        # 終了処理
        controller.quit()
        print("ゲーム終了")
        
    except Exception as e:
        print(f"エラーが発生しました: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())