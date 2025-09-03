#!/usr/bin/env python3
"""
ゲーム制御システム
キーボード入力とゲーム状態管理
"""

import pygame
import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

import puyo_ai_platform as pap
from python.ui.renderer import GameVisualizer
from python.ui.player_controller import HumanPlayerController, AIPlayerController


class GameController:
    """メインゲーム制御クラス"""
    
    def __init__(self, game_mode=pap.GameMode.TOKOTON, player_controller=None):
        self.game_manager = pap.GameManager(game_mode)
        self.visualizer = GameVisualizer()
        self.player_controller = player_controller  # PlayerController instance
        
        # 操作中のぷよペア
        self.current_pair = None
        self.pair_placed = False
        self.puyo_controller = None  # C++のPuyoControllerを使用
        self.next_generator = None   # C++のNextGeneratorを使用
        
        # 連鎖システム
        self.last_chain_count = 0
        self.last_score_details = None  # 最新のスコア詳細
        
        # ゲーム設定（エミュレータ方式：時間経過による落下なし）
        
        # デバッグフラグ
        self.debug_mode = True
        
        # AI制御関連
        self.turn_count = 0
        self.ai_mode = False
        
    def setup_game(self, player_names=None):
        """ゲーム初期設定"""
        if player_names is None:
            player_names = ["Player1"]
        
        # PlayerControllerタイプに基づいてPlayerTypeを決定
        player_type = pap.PlayerType.HUMAN
        if self.player_controller and isinstance(self.player_controller, AIPlayerController):
            player_type = pap.PlayerType.AI
            self.ai_mode = True
        
        # プレイヤー追加
        for name in player_names:
            self.game_manager.add_player(name, player_type)
            if self.debug_mode:
                print(f"Added player: {name} (Type: {player_type})")
        
        # 入力コールバック設定
        self.game_manager.set_input_callback(self._input_callback)
        
        # PuyoControllerを初期化（現在のプレイヤーのフィールドを使用）
        current_player = self.game_manager.get_player(0)
        if current_player:
            field = current_player.get_field()
            self.puyo_controller = pap.PuyoController(field)
            
            # NextGeneratorを初期化
            self.next_generator = current_player.get_next_generator()
            # 4色設定
            colors = [pap.PuyoColor.RED, pap.PuyoColor.GREEN, pap.PuyoColor.BLUE, pap.PuyoColor.YELLOW]
            self.next_generator.set_active_colors(colors)
            self.next_generator.initialize_next_sequence()
        
        # NextGeneratorから現在のペアを取得
        self._get_current_pair_from_next_generator()
        
        if self.debug_mode:
            print("Game setup completed")
    
    def _input_callback(self, _player_id):
        """ゲームマネージャーからのコールバック"""
        # 現在は手動制御のため、常にNONEを返す
        return pap.MoveCommand.NONE
    
    def _get_current_pair_from_next_generator(self):
        """NextGeneratorから現在のぷよペアを取得"""
        if not self.next_generator:
            return
        
        # NextGeneratorから現在のペアを取得
        self.current_pair = self.next_generator.get_current_pair()
        self.pair_placed = False
        
        # C++のPuyoControllerに現在のペアを設定
        if self.puyo_controller:
            self.puyo_controller.set_current_pair(self.current_pair)
        
        if self.debug_mode:
            print(f"New pair from NextGenerator: {self.current_pair.axis} + {self.current_pair.child}")
    
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
        
        # 軸ぷよの落下距離を計算（ドロップボーナス用）
        drop_height = 12 - self.current_pair.pos.y  # 12段目からの距離
        
        # C++のPuyoControllerでぷよペア設置
        if self.puyo_controller.place_current_pair():
            self.pair_placed = True
            
            # 重力適用
            current_player_id = self.game_manager.get_current_player()
            current_player = self.game_manager.get_player(current_player_id)
            if current_player:
                field = current_player.get_field()
                field.apply_gravity()
                
                # 落下ボーナス付きで連鎖処理実行
                chain_system = current_player.get_chain_system()
                chain_result = chain_system.execute_chains_with_drop_bonus(drop_height)
                
                # スコア詳細を保存（UI表示用）
                self.last_score_details = chain_result.score_result
                
                # プレイヤー統計を更新
                if chain_result.score_result.total_score > 0:
                    stats = current_player.get_stats()
                    stats.total_score += chain_result.score_result.total_score
                    
                    # 連鎖統計の更新
                    if chain_result.has_chains():
                        stats.total_chains += chain_result.total_chains
                        if chain_result.total_chains > stats.max_chain:
                            stats.max_chain = chain_result.total_chains
                
                # 連鎖結果を記録・表示
                if chain_result.has_chains():
                    self.last_chain_count = chain_result.total_chains
                    if self.debug_mode:
                        print(f"Chain executed! Total chains: {chain_result.total_chains}")
                        print(f"Score details - Chain: {chain_result.score_result.chain_score}, Drop: {chain_result.score_result.drop_score}, All Clear: {chain_result.score_result.all_clear_bonus}")
                        print(f"Total score gained: {chain_result.score_result.total_score}")
                else:
                    self.last_chain_count = 0
                    if self.debug_mode and chain_result.score_result.drop_score > 0:
                        print(f"No chains, but drop bonus: {chain_result.score_result.drop_score}")
                    elif self.debug_mode:
                        print("No chains occurred")
            
            # NextGeneratorを進めて次のペア取得
            if self.next_generator:
                self.next_generator.advance_to_next()
                self._get_current_pair_from_next_generator()
            
            if self.debug_mode:
                print("Pair placed, generated new pair")
            
            return True
        
        return False
    
    # 自動落下処理は削除（エミュレータ方式のため）
    
    def handle_input(self, key):
        """キー入力処理"""
        # 共通キーの処理
        if key == pygame.K_ESCAPE:
            return False  # 終了
        
        if key == pygame.K_r:
            self.reset_game()
            return True
        
        # PlayerControllerに入力を委譲
        if self.player_controller and isinstance(self.player_controller, HumanPlayerController):
            self.player_controller.set_key_input(key)
        
        return True
    
    def reset_game(self):
        """ゲームリセット"""
        self.game_manager.reset_game()
        
        # スコア詳細もリセット
        self.last_score_details = None
        self.last_chain_count = 0
        self.turn_count = 0
        
        # PlayerControllerもリセット
        if self.player_controller:
            self.player_controller.reset()
        
        # NextGeneratorを再初期化
        if self.next_generator:
            self.next_generator.initialize_next_sequence()
            self._get_current_pair_from_next_generator()
        
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
        
        # PlayerController経由でコマンド取得と実行
        self._process_player_input()
        
        return True
    
    def render(self):
        """ゲーム描画"""
        highlight = not self.pair_placed
        
        # AI情報の構築
        ai_info = None
        if self.ai_mode and self.player_controller:
            ai_instance = getattr(self.player_controller, 'ai', None)
            ai_name = 'Unknown AI'
            if ai_instance and hasattr(ai_instance, 'get_name'):
                ai_name = ai_instance.get_name()
            
            ai_info = {
                'ai_name': ai_name,
                'last_command': getattr(self.player_controller, 'last_command', 'None'),
                'think_time': getattr(self.player_controller, 'last_think_time', 0.0) * 1000  # ms変換
            }
        
        self.visualizer.render_game(self.game_manager, self.current_pair, highlight, 
                                   self.last_chain_count, self.last_score_details, 
                                   self.ai_mode, ai_info)
    
    def handle_events(self):
        """イベント処理"""
        key = self.visualizer.handle_events()
        if key is not None:
            return self.handle_input(key)
        return self.visualizer.is_running()
    
    def is_running(self):
        """実行中判定"""
        return self.visualizer.is_running()
    
    def _process_player_input(self):
        """PlayerController経由で入力処理"""
        if not self.player_controller or self.pair_placed:
            return
        
        # ゲーム状態を構築してPlayerControllerに渡す
        game_state = self._build_game_state()
        
        # PlayerControllerからコマンド取得
        command = self.player_controller.get_next_command(game_state)
        
        if command:
            self._try_move_pair(command)
    
    def _build_game_state(self):
        """PlayerController用のゲーム状態情報を構築"""
        current_player_id = self.game_manager.get_current_player()
        current_player = self.game_manager.get_player(current_player_id)
        
        return {
            'current_player': current_player,
            'current_pair': self.current_pair,
            'turn_count': self.turn_count,
            'pair_placed': self.pair_placed,
            'last_chain_count': self.last_chain_count,
            'last_score_details': self.last_score_details
        }
    
    def quit(self):
        """終了処理"""
        self.visualizer.quit()

def main():
    """メイン実行関数"""
    print("Puyo Puyo UI Demo Starting...")
    print("Select mode: [1] Human Player [2] AI Player")
    
    # プレイヤータイプ選択
    try:
        choice = input("Enter choice (1 or 2): ").strip()
        if choice == "2":
            # AI制御モード
            print("AI Mode Selected")
            
            # 利用可能なAI一覧を表示
            ai_manager = pap.ai.get_global_ai_manager()
            ai_names = ai_manager.get_registered_ai_names()
            
            print("Available AIs:")
            for i, ai_name in enumerate(ai_names):
                print(f"  [{i+1}] {ai_name}")
            
            # AI選択
            try:
                ai_choice = input(f"Select AI (1-{len(ai_names)}): ").strip()
                ai_index = int(ai_choice) - 1
                
                if 0 <= ai_index < len(ai_names):
                    selected_ai_name = ai_names[ai_index]
                    print(f"Selected AI: {selected_ai_name}")
                    
                    ai_instance = ai_manager.create_ai(selected_ai_name)
                    if ai_instance is None:
                        print("Failed to create AI, falling back to RandomAI")
                        ai_instance = ai_manager.create_ai("random")
                else:
                    print("Invalid AI selection, using RandomAI")
                    ai_instance = ai_manager.create_ai("random")
                    selected_ai_name = "random"
            except (ValueError, EOFError, KeyboardInterrupt):
                print("Invalid input, using RandomAI")
                ai_instance = ai_manager.create_ai("random")
                selected_ai_name = "random"
            
            player_controller = AIPlayerController(ai_instance, f"{selected_ai_name} AI")
            print(f"Game started with {selected_ai_name} AI. R to reset, ESC to quit.")
        else:
            # 人間制御モード（デフォルト）
            print("Human Mode Selected")
            player_controller = HumanPlayerController("Human Player")
            print("Game loop started. Use A/D to move, ↓/→ to rotate, W to drop, R to reset, ESC to quit.")
    except (EOFError, KeyboardInterrupt):
        print("Human Mode Selected (default)")
        player_controller = HumanPlayerController("Human Player")
        print("Game loop started. Use A/D to move, ↓/→ to rotate, W to drop, R to reset, ESC to quit.")
    
    # ゲームコントローラー初期化
    controller = GameController(pap.GameMode.TOKOTON, player_controller)
    controller.setup_game([player_controller.get_player_name()])
    
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