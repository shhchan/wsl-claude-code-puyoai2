#!/usr/bin/env python3
"""
プレイヤー制御抽象化システム
人間制御とAI制御の統一インターフェース
"""

import time
from abc import ABC, abstractmethod
from typing import Optional, Dict, Any
import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

import puyo_ai_platform as pap


class PlayerController(ABC):
    """プレイヤー制御の抽象基底クラス"""
    
    def __init__(self, player_name: str, controller_type: str):
        self.player_name = player_name
        self.controller_type = controller_type
        self.debug_mode = True
    
    @abstractmethod
    def get_next_command(self, game_state: Dict[str, Any]) -> Optional[pap.MoveCommand]:
        """次の行動を取得する（抽象メソッド）"""
        pass
    
    @abstractmethod
    def should_place_pair(self, game_state: Dict[str, Any]) -> bool:
        """ぷよペアを設置すべきかの判定（抽象メソッド）"""
        pass
    
    @abstractmethod
    def on_pair_placed(self, game_state: Dict[str, Any]) -> None:
        """ぷよペア設置後の処理（抽象メソッド）"""
        pass
    
    @abstractmethod
    def reset(self) -> None:
        """コントローラーリセット（抽象メソッド）"""
        pass
    
    def get_type(self) -> str:
        """制御タイプを返す"""
        return self.controller_type
    
    def get_player_name(self) -> str:
        """プレイヤー名を返す"""
        return self.player_name


class InputManager:
    """キーボード入力管理クラス（HumanPlayerController用）"""
    
    def __init__(self):
        import pygame
        self.pygame = pygame
        
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
        self.pending_command = None
    
    def update(self, key_input: Optional[int]) -> Optional[pap.MoveCommand]:
        """キー入力からMoveCommandを生成"""
        if key_input is None:
            return None
        
        current_time = time.time()
        
        # キーリピート防止チェック
        """
        if key_input in self.last_key_time:
            if current_time - self.last_key_time[key_input] < self.key_repeat_delay:
                return None
        """
        
        self.last_key_time[key_input] = current_time
        
        return self.key_mapping.get(key_input, None)
    
    def is_quit_key(self, key: int) -> bool:
        """終了キーかチェック"""
        return key == self.pygame.K_ESCAPE
    
    def is_reset_key(self, key: int) -> bool:
        """リセットキーかチェック"""
        return key == self.pygame.K_r


class HumanPlayerController(PlayerController):
    """人間プレイヤー制御クラス"""
    
    def __init__(self, player_name: str = "Human"):
        super().__init__(player_name, "Human")
        self.input_manager = InputManager()
        self.pending_key = None
        self.last_command_time = 0.0
    
    def set_key_input(self, key: Optional[int]) -> None:
        """外部からキー入力を設定"""
        self.pending_key = key
    
    def get_next_command(self, game_state: Dict[str, Any]) -> Optional[pap.MoveCommand]:
        """キー入力からコマンドを取得"""
        if self.pending_key is None:
            return None
        
        # InputManagerを使ってコマンドに変換
        command = self.input_manager.update(self.pending_key)
        self.pending_key = None  # 1回使ったらクリア
        
        if command and self.debug_mode:
            print(f"Human input command: {command}")
        
        return command
    
    def should_place_pair(self, game_state: Dict[str, Any]) -> bool:
        """人間制御では手動配置なのでFalseを返す"""
        return False
    
    def on_pair_placed(self, game_state: Dict[str, Any]) -> None:
        """ぷよペア設置後の処理（人間制御では特に処理なし）"""
        if self.debug_mode:
            print(f"Human player placed pair")
    
    def reset(self) -> None:
        """リセット処理"""
        self.pending_key = None
        self.input_manager.last_key_time.clear()


class AIPlayerController(PlayerController):
    """AIプレイヤー制御クラス"""
    
    def __init__(self, ai_instance, player_name: str = "AI"):
        super().__init__(player_name, "AI")
        self.ai = ai_instance
        self.last_think_time = 0.0
        self.think_interval = 0.3  # 300ms間隔で思考
        self.pending_command = None
        self.command_queue = []
        self.last_command = 'None'  # 最後に実行したコマンドを記録
        
        # AI初期化
        if hasattr(self.ai, 'initialize'):
            self.ai.initialize()
    
    def get_next_command(self, game_state: Dict[str, Any]) -> Optional[pap.MoveCommand]:
        """AIから次のコマンドを取得"""
        current_time = time.time()
        
        # キューにコマンドがある場合は順次実行
        if self.command_queue:
            command = self.command_queue.pop(0)
            self.last_command = str(command)
            if self.debug_mode:
                print(f"AI executing queued command: {command} (remaining: {len(self.command_queue)})")
            return command
        
        # 思考時間間隔チェック
        """
        if current_time - self.last_think_time < self.think_interval:
            return None
        """
        self.last_think_time = current_time
        
        try:
            # GameStateを構築
            ai_game_state = self._build_ai_game_state(game_state)
            
            # AI思考実行
            think_start_time = time.time()
            decision = self.ai.think(ai_game_state)
            think_duration = time.time() - think_start_time
            
            # 新しいAIDecision構造体に対応
            if decision and hasattr(decision, 'move_commands'):
                # move_commandsリストをキューに追加
                if hasattr(decision, 'move_commands') and decision.move_commands:
                    self.command_queue = list(decision.move_commands)
                    
                    if self.debug_mode:
                        print(f"AI decision: ({decision.x}, {decision.r}) with {len(decision.move_commands)} commands")
                        if hasattr(decision, 'reason'):
                            print(f"AI reason: {decision.reason}")
                        print(f"AI think time: {think_duration*1000:.1f}ms")
                        command_str = ' '.join([str(cmd).split('.')[-1] for cmd in decision.move_commands])
                        print(f"Command sequence: {command_str}")
                    
                    # 最初のコマンドを返す
                    if self.command_queue:
                        command = self.command_queue.pop(0)
                        self.last_command = str(command)
                        if self.debug_mode:
                            print(f"AI executing first command: {command} (remaining: {len(self.command_queue)})")
                        return command
            
            # 古いAIDecision構造体との後方互換性（エラー回避）
            elif decision and hasattr(decision, 'command'):
                command = decision.command
                
                if self.debug_mode:
                    print(f"AI decision (legacy): {command}")
                    if hasattr(decision, 'reason'):
                        print(f"AI reason: {decision.reason}")
                
                self.last_command = str(command)
                return command
        
        except Exception as e:
            if self.debug_mode:
                print(f"AI think error: {e}")
                import traceback
                traceback.print_exc()
            
            # エラー時はランダムコマンドを返す
            import random
            commands = [pap.MoveCommand.LEFT, pap.MoveCommand.RIGHT, 
                       pap.MoveCommand.ROTATE_CW, pap.MoveCommand.ROTATE_CCW, 
                       pap.MoveCommand.DROP]
            command = random.choice(commands)
            self.last_command = f"ERROR_FALLBACK_{command}"
            return command
        
        return None
    
    def should_place_pair(self, game_state: Dict[str, Any]) -> bool:
        """AI制御では自動でDROPコマンドを送るのでFalseを返す"""
        return False
    
    def on_pair_placed(self, game_state: Dict[str, Any]) -> None:
        """ぷよペア設置後の処理"""
        if self.debug_mode:
            print(f"AI player placed pair")
    
    def reset(self) -> None:
        """リセット処理"""
        self.pending_command = None
        self.command_queue.clear()
        self.last_think_time = 0.0
        self.last_command = 'None'
    
    def _build_ai_game_state(self, game_state: Dict[str, Any]):
        """GameControllerの情報からAIのGameStateを構築"""
        # C++のGameStateを構築
        ai_state = pap.ai.GameState()
        
        # 基本情報を設定
        ai_state.player_id = 0  # 単体プレイなので固定
        ai_state.turn_count = game_state.get('turn_count', 0)
        ai_state.is_versus_mode = False
        
        # フィールド情報を設定
        if 'current_player' in game_state and game_state['current_player']:
            player = game_state['current_player']
            # own_fieldを設定 - RandomAIがフィールド情報を使用するため
            try:
                # 新しく追加したset_own_fieldメソッドを使用
                field = player.get_field()
                ai_state.set_own_field(field)
                if self.debug_mode:
                    print(f"AI GameState: own_field set successfully via set_own_field()")
            except Exception as e:
                if self.debug_mode:
                    print(f"Warning: Failed to set own_field: {e}")
                    import traceback
                    traceback.print_exc()
        
        # 現在のぷよペア情報を設定
        if 'current_pair' in game_state and game_state['current_pair']:
            ai_state.current_pair = game_state['current_pair']
            if self.debug_mode:
                pair = game_state['current_pair']
                print(f"AI GameState: current_pair set ({pair.axis} + {pair.child})")
        
        return ai_state