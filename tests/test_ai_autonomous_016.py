#!/usr/bin/env python3
"""
チケット016: AI自律プレイシステム統合テスト
AIが自律的にとことんモードでプレイできることを検証
"""

import sys
import os
import time
import unittest
from unittest.mock import Mock, patch

# パス設定
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))

import puyo_ai_platform as pap
from python.ui.game_controller import GameController
from python.ui.player_controller import HumanPlayerController, AIPlayerController


class TestAIAutonomousPlay(unittest.TestCase):
    """AI自律プレイテストケース"""
    
    def setUp(self):
        """テストセットアップ"""
        self.ai_manager = pap.ai.AIManager()
        # RandomAIは既に登録済み
    
    def test_ai_manager_functionality(self):
        """AIマネージャーの機能テスト"""
        # AI作成テスト
        ai_instance = self.ai_manager.create_ai("random")
        self.assertIsNotNone(ai_instance)
        self.assertTrue(ai_instance.initialize())
        
        # AI基本情報テスト
        self.assertEqual(ai_instance.get_type(), "Random")
        self.assertEqual(ai_instance.get_name(), "RandomAI")
        self.assertTrue(ai_instance.is_initialized())
    
    def test_ai_decision_making(self):
        """AI判断機能テスト"""
        ai_instance = self.ai_manager.create_ai("random")
        ai_instance.initialize()
        
        # ゲーム状態を構築
        game_state = pap.ai.GameState()
        game_state.player_id = 0
        game_state.turn_count = 1
        game_state.is_versus_mode = False
        
        # AI思考テスト
        decision = ai_instance.think(game_state)
        self.assertIsNotNone(decision)
        self.assertIn(decision.command, [
            pap.MoveCommand.LEFT, pap.MoveCommand.RIGHT,
            pap.MoveCommand.ROTATE_CW, pap.MoveCommand.ROTATE_CCW,
            pap.MoveCommand.DROP
        ])
        self.assertGreaterEqual(decision.confidence, 0.0)
        self.assertLessEqual(decision.confidence, 1.0)
    
    def test_human_player_controller(self):
        """HumanPlayerController基本機能テスト"""
        controller = HumanPlayerController("Test Human")
        
        # 基本情報テスト
        self.assertEqual(controller.get_type(), "Human")
        self.assertEqual(controller.get_player_name(), "Test Human")
        
        # キー入力テスト（模擬）
        import pygame
        pygame.init()
        
        controller.set_key_input(pygame.K_a)
        game_state = {}
        command = controller.get_next_command(game_state)
        self.assertEqual(command, pap.MoveCommand.LEFT)
        
        # リセットテスト
        controller.reset()
        self.assertIsNone(controller.pending_key)
        
        pygame.quit()
    
    def test_ai_player_controller(self):
        """AIPlayerController基本機能テスト"""
        ai_instance = self.ai_manager.create_ai("random")
        controller = AIPlayerController(ai_instance, "Test AI")
        
        # 基本情報テスト
        self.assertEqual(controller.get_type(), "AI")
        self.assertEqual(controller.get_player_name(), "Test AI")
        
        # AI統合テスト
        self.assertIsNotNone(controller.ai)
        self.assertTrue(controller.ai.is_initialized())
        
        # ゲーム状態構築と判断テスト
        mock_game_state = {
            'current_player': Mock(),
            'current_pair': Mock(),
            'turn_count': 1
        }
        
        # Fieldのモック設定
        mock_field = Mock()
        mock_game_state['current_player'].get_field.return_value = mock_field
        
        # AIコマンド取得テスト（時間制限により None が返される可能性）
        start_time = time.time()
        command = controller.get_next_command(mock_game_state)
        elapsed = time.time() - start_time
        
        # 最初の呼び出しは時間制限でNoneになる可能性
        if command is None:
            # 時間間隔を待って再試行
            time.sleep(0.4)  # think_interval (0.3s) より少し長く待機
            command = controller.get_next_command(mock_game_state)
        
        # 2回目の呼び出しではコマンドが取得できるはず
        if command is not None:
            self.assertIn(command, [
                pap.MoveCommand.LEFT, pap.MoveCommand.RIGHT,
                pap.MoveCommand.ROTATE_CW, pap.MoveCommand.ROTATE_CCW,
                pap.MoveCommand.DROP, pap.MoveCommand.NONE
            ])
        
        # リセットテスト
        controller.reset()
        self.assertEqual(len(controller.command_queue), 0)
    
    @patch('pygame.init')
    @patch('pygame.display.set_mode')
    @patch('pygame.font.Font')
    @patch('pygame.time.Clock')
    def test_game_controller_human_mode(self, mock_clock, mock_font, mock_display, mock_init):
        """GameController人間モードテスト"""
        # HumanPlayerControllerでGameController初期化
        human_controller = HumanPlayerController("Human Test")
        game_controller = GameController(pap.GameMode.TOKOTON, human_controller)
        
        # セットアップテスト
        game_controller.setup_game(["Human Test Player"])
        
        # モード確認
        self.assertFalse(game_controller.ai_mode)
        self.assertIsInstance(game_controller.player_controller, HumanPlayerController)
        
        # ゲーム状態構築テスト
        game_state = game_controller._build_game_state()
        self.assertIn('current_player', game_state)
        self.assertIn('turn_count', game_state)
    
    @patch('pygame.init')
    @patch('pygame.display.set_mode')
    @patch('pygame.font.Font')
    @patch('pygame.time.Clock')
    def test_game_controller_ai_mode(self, mock_clock, mock_font, mock_display, mock_init):
        """GameController AIモードテスト"""
        # AIPlayerControllerでGameController初期化
        ai_instance = self.ai_manager.create_ai("random")
        ai_controller = AIPlayerController(ai_instance, "AI Test")
        game_controller = GameController(pap.GameMode.TOKOTON, ai_controller)
        
        # セットアップテスト
        game_controller.setup_game(["AI Test Player"])
        
        # モード確認
        self.assertTrue(game_controller.ai_mode)
        self.assertIsInstance(game_controller.player_controller, AIPlayerController)
        
        # ゲーム状態構築テスト
        game_state = game_controller._build_game_state()
        self.assertIn('current_player', game_state)
        self.assertIn('turn_count', game_state)
    
    def test_ai_game_state_conversion(self):
        """AIゲーム状態変換テスト"""
        ai_instance = self.ai_manager.create_ai("random")
        controller = AIPlayerController(ai_instance, "AI Test")
        
        # 実際のゲーム状態を作成（モックではなく）
        game_manager = pap.GameManager(pap.GameMode.TOKOTON)
        game_manager.add_player("Test Player", pap.PlayerType.AI)
        player = game_manager.get_player(0)
        
        # 実際のPuyoPairを作成
        real_pair = pap.PuyoPair(
            pap.PuyoColor.RED, pap.PuyoColor.BLUE, 
            pap.Position(2, 11), pap.Rotation.UP
        )
        
        game_state = {
            'current_player': player,
            'current_pair': real_pair,
            'turn_count': 5
        }
        
        # AI用GameState変換
        ai_state = controller._build_ai_game_state(game_state)
        
        # 変換結果確認
        self.assertEqual(ai_state.current_pair.axis, real_pair.axis)
        self.assertEqual(ai_state.current_pair.child, real_pair.child)
        self.assertEqual(ai_state.turn_count, 5)
        self.assertEqual(ai_state.player_id, 0)
        self.assertFalse(ai_state.is_versus_mode)


class TestAIAutonomousIntegration(unittest.TestCase):
    """AI自律プレイ統合テストケース"""
    
    def setUp(self):
        """統合テストセットアップ"""
        self.ai_manager = pap.ai.AIManager()
        # RandomAIは既に登録済み
    
    @patch('pygame.init')
    @patch('pygame.display.set_mode')
    @patch('pygame.font.Font')
    @patch('pygame.time.Clock')
    def test_ai_autonomous_game_simulation(self, mock_clock, mock_font, mock_display, mock_init):
        """AI自律ゲームシミュレーションテスト"""
        # AI作成とコントローラー初期化
        ai_instance = self.ai_manager.create_ai("random")
        ai_controller = AIPlayerController(ai_instance, "Simulation AI")
        
        # GameController初期化
        game_controller = GameController(pap.GameMode.TOKOTON, ai_controller)
        game_controller.setup_game(["Simulation AI"])
        
        # AI制御モード確認
        self.assertTrue(game_controller.ai_mode)
        
        # 短時間のシミュレーション実行
        simulation_steps = 5
        for step in range(simulation_steps):
            if not game_controller.update():
                break
            
            # AI入力処理確認
            game_state = game_controller._build_game_state()
            self.assertIsNotNone(game_state)
            
            # 少し待機（AI思考時間間隔）
            time.sleep(0.1)
        
        # シミュレーション完了確認
        self.assertTrue(True)  # エラーなく完了することが重要
    
    def test_mode_switching_consistency(self):
        """モード切り替え一貫性テスト"""
        # Human -> AI切り替えテスト
        ai_instance = self.ai_manager.create_ai("random")
        
        # 人間コントローラー
        human_controller = HumanPlayerController("Human")
        self.assertEqual(human_controller.get_type(), "Human")
        
        # AIコントローラー
        ai_controller = AIPlayerController(ai_instance, "AI")
        self.assertEqual(ai_controller.get_type(), "AI")
        
        # 異なるコントローラータイプの確認
        self.assertNotEqual(human_controller.get_type(), ai_controller.get_type())


def run_comprehensive_tests():
    """包括的テスト実行"""
    print("=== チケット016: AI自律プレイシステム 統合テスト ===")
    print()
    
    # テストスイート構築
    test_suite = unittest.TestSuite()
    
    # 基本機能テスト
    test_suite.addTest(unittest.makeSuite(TestAIAutonomousPlay))
    
    # 統合テスト
    test_suite.addTest(unittest.makeSuite(TestAIAutonomousIntegration))
    
    # テスト実行
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(test_suite)
    
    # 結果サマリー
    print(f"\n=== テスト結果サマリー ===")
    print(f"実行テスト数: {result.testsRun}")
    print(f"成功: {result.testsRun - len(result.failures) - len(result.errors)}")
    print(f"失敗: {len(result.failures)}")
    print(f"エラー: {len(result.errors)}")
    
    if result.failures:
        print("\n失敗したテスト:")
        for test, traceback in result.failures:
            print(f"- {test}: {traceback.split('AssertionError:')[-1].strip()}")
    
    if result.errors:
        print("\nエラーが発生したテスト:")
        for test, traceback in result.errors:
            print(f"- {test}: {traceback.split('Exception:')[-1].strip()}")
    
    success_rate = (result.testsRun - len(result.failures) - len(result.errors)) / result.testsRun * 100
    print(f"\n成功率: {success_rate:.1f}%")
    
    return result.wasSuccessful(), success_rate


if __name__ == "__main__":
    success, rate = run_comprehensive_tests()
    sys.exit(0 if success else 1)