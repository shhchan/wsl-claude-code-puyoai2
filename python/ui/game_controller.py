#!/usr/bin/env python3
"""
ゲーム制御システム
キーボード入力とゲーム状態管理
"""

import pygame
import sys
import os
import time
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

import puyo_ai_platform as pap
from python.ui.renderer import GameVisualizer

class InputManager:
    """キーボード入力管理クラス"""
    
    def __init__(self):
        self.key_mapping = {
            pygame.K_a: pap.MoveCommand.LEFT,
            pygame.K_d: pap.MoveCommand.RIGHT,
            pygame.K_DOWN: pap.MoveCommand.ROTATE_CCW,   # ↓ 反時計回り
            pygame.K_RIGHT: pap.MoveCommand.ROTATE_CW,   # → 時計回り
            pygame.K_w: pap.MoveCommand.DROP,            # W ドロップ
        }
        
        # キーリピート防止
        self.last_key_time = {}
        self.key_repeat_delay = 0.15  # 150ms
    
    def get_move_command(self, key):
        """キー入力をMoveCommandに変換"""
        current_time = time.time()
        
        # キーリピート防止チェック
        if key in self.last_key_time:
            if current_time - self.last_key_time[key] < self.key_repeat_delay:
                return None
        
        self.last_key_time[key] = current_time
        
        return self.key_mapping.get(key, None)
    
    def is_quit_key(self, key):
        """終了キーかチェック"""
        return key == pygame.K_ESCAPE
    
    def is_reset_key(self, key):
        """リセットキーかチェック"""
        return key == pygame.K_r

class GameController:
    """メインゲーム制御クラス"""
    
    def __init__(self, game_mode=pap.GameMode.TOKOTON):
        self.game_manager = pap.GameManager(game_mode)
        self.visualizer = GameVisualizer()
        self.input_manager = InputManager()
        
        # 操作中のぷよペア
        self.current_pair = None
        self.pair_placed = False
        self.puyo_controller = None  # C++のPuyoControllerを使用
        
        # ゲーム設定（エミュレータ方式：時間経過による落下なし）
        
        # デバッグフラグ
        self.debug_mode = True
        
    def setup_game(self, player_names=None):
        """ゲーム初期設定"""
        if player_names is None:
            player_names = ["Player1"]
        
        # プレイヤー追加
        for i, name in enumerate(player_names):
            self.game_manager.add_player(name, pap.PlayerType.HUMAN)
            if self.debug_mode:
                print(f"Added player: {name}")
        
        # 入力コールバック設定
        self.game_manager.set_input_callback(self._input_callback)
        
        # PuyoControllerを初期化（現在のプレイヤーのフィールドを使用）
        current_player = self.game_manager.get_player(0)
        if current_player:
            field = current_player.get_field()
            self.puyo_controller = pap.PuyoController(field)
        
        # 初期ぷよペア生成
        self._generate_new_pair()
        
        if self.debug_mode:
            print("Game setup completed")
    
    def _input_callback(self, player_id):
        """ゲームマネージャーからのコールバック"""
        # 現在は手動制御のため、常にNONEを返す
        return pap.MoveCommand.NONE
    
    def _generate_new_pair(self):
        """新しいぷよペアを生成"""
        # とりあえず固定ペアを生成（本来はNextGeneratorから取得）
        colors = [pap.PuyoColor.RED, pap.PuyoColor.GREEN, pap.PuyoColor.BLUE, pap.PuyoColor.YELLOW]
        import random
        
        axis_color = random.choice(colors)
        child_color = random.choice(colors)
        
        self.current_pair = pap.PuyoPair(
            axis=axis_color,
            child=child_color,
            pos=pap.Position(2, 11),  # 3列目、12段目から開始
            rot=pap.Rotation.UP
        )
        self.pair_placed = False
        
        # C++のPuyoControllerに現在のペアを設定
        if self.puyo_controller:
            self.puyo_controller.set_current_pair(self.current_pair)
        
        if self.debug_mode:
            print(f"New pair: {axis_color} + {child_color}")
    
    def _try_move_pair(self, command):
        """ぷよペアの移動試行（C++のPuyoControllerを使用）"""
        if not self.current_pair or self.pair_placed or not self.puyo_controller:
            return False
        
        # DROPコマンドの特殊処理（底まで落下）
        if command == pap.MoveCommand.DROP:
            self._drop_pair_to_bottom()
            return True
        
        # C++のPuyoControllerで操作実行（壁キック処理含む）
        success = self.puyo_controller.execute_command(command)
        
        if success:
            # PuyoControllerから更新されたペア状態を取得
            self.current_pair = self.puyo_controller.get_current_pair()
            
            if self.debug_mode and command in [pap.MoveCommand.ROTATE_CW, pap.MoveCommand.ROTATE_CCW]:
                print(f"Rotation executed with wall kick support")
        else:
            if self.debug_mode and command in [pap.MoveCommand.ROTATE_CW, pap.MoveCommand.ROTATE_CCW]:
                print(f"Rotation failed - no valid rotation or kick possible")
        
        return success
    
    def _drop_pair_to_bottom(self):
        """ぷよペアを底まで落下（C++のPuyoControllerを使用）"""
        if not self.current_pair or not self.puyo_controller:
            return
        
        # C++のPuyoControllerで下移動を繰り返して底まで落下
        while self.puyo_controller.can_move_down():
            if not self.puyo_controller.move_down():
                break
            # Python側の現在ペアも更新
            self.current_pair = self.puyo_controller.get_current_pair()
        
        # 底に到達したので設置
        self._place_current_pair()
    
    def _place_current_pair(self):
        """現在のぷよペアを設置（C++のPuyoControllerを使用）"""
        if not self.current_pair or self.pair_placed or not self.puyo_controller:
            return False
        
        # C++のPuyoControllerでぷよペア設置
        if self.puyo_controller.place_current_pair():
            self.pair_placed = True
            
            # 重力適用
            current_player_id = self.game_manager.get_current_player()
            current_player = self.game_manager.get_player(current_player_id)
            if current_player:
                field = current_player.get_field()
                field.apply_gravity()
            
            # 次のペア生成
            self._generate_new_pair()
            
            if self.debug_mode:
                print("Pair placed, generated new pair")
            
            return True
        
        return False
    
    # 自動落下処理は削除（エミュレータ方式のため）
    
    def handle_input(self, key):
        """キー入力処理"""
        if self.input_manager.is_quit_key(key):
            return False  # 終了
        
        if self.input_manager.is_reset_key(key):
            self.reset_game()
            return True
        
        # 移動コマンド処理
        command = self.input_manager.get_move_command(key)
        if command:
            self._try_move_pair(command)
        
        return True
    
    def reset_game(self):
        """ゲームリセット"""
        self.game_manager.reset_game()
        self._generate_new_pair()
        
        if self.debug_mode:
            print("Game reset")
    
    def update(self):
        """ゲーム状態更新"""
        # ゲーム終了判定
        current_player_id = self.game_manager.get_current_player()
        current_player = self.game_manager.get_player(current_player_id)
        if current_player and current_player.is_game_over():
            if self.debug_mode:
                print("Game Over!")
            return False
        
        return True
    
    def render(self):
        """ゲーム描画"""
        highlight = not self.pair_placed
        self.visualizer.render_game(self.game_manager, self.current_pair, highlight)
    
    def handle_events(self):
        """イベント処理"""
        key = self.visualizer.handle_events()
        if key is not None:
            return self.handle_input(key)
        return self.visualizer.is_running()
    
    def is_running(self):
        """実行中判定"""
        return self.visualizer.is_running()
    
    def quit(self):
        """終了処理"""
        self.visualizer.quit()

def main():
    """メイン実行関数"""
    print("Puyo Puyo UI Demo Starting...")
    
    # ゲームコントローラー初期化
    controller = GameController(pap.GameMode.TOKOTON)
    controller.setup_game(["Human Player"])
    
    print("Game loop started. Use A/D to move, ↓/→ to rotate, W to drop, R to reset, ESC to quit.")
    
    # メインループ
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
    print("Game ended.")

if __name__ == "__main__":
    main()