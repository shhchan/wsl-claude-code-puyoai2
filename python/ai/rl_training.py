#!/usr/bin/env python3
"""
RLPlayerAI事前学習システム
Q-Learning with Experience Replayによる強化学習実装
"""

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

import puyo_ai_platform as pap
import numpy as np
import random
import json
import yaml
from collections import deque
import time
from datetime import datetime
import pickle


class RLTrainingEnvironment:
    """強化学習用のぷよぷよ環境"""
    
    def __init__(self):
        self.game_manager = None
        self.reset()
    
    def reset(self):
        """環境のリセット"""
        self.game_manager = pap.GameManager(pap.GameMode.TOKOTON)
        self.game_manager.add_player("RL Player", pap.PlayerType.HUMAN)  # 手動制御
        
        # 基本設定
        player = self.game_manager.get_player(0)
        if player:
            # NextGeneratorの設定
            next_gen = player.get_next_generator()
            colors = [pap.PuyoColor.RED, pap.PuyoColor.GREEN, 
                     pap.PuyoColor.BLUE, pap.PuyoColor.YELLOW]
            next_gen.set_active_colors(colors)
            next_gen.initialize_next_sequence()
        
        return self.get_state()
    
    def get_state(self):
        """現在の状態を取得"""
        player = self.game_manager.get_player(0)
        if not player:
            return None
        
        field = player.get_field()
        next_gen = player.get_next_generator()
        
        # フィールド状態のエンコーディング（簡易版）
        field_state = []
        for y in range(13):  # FIELD_HEIGHT
            for x in range(6):  # FIELD_WIDTH
                color = field.get_puyo(pap.Position(x, y))
                field_state.append(int(color))
        
        # 現在のぷよペア
        current_pair = next_gen.get_current_pair()
        current_colors = [int(current_pair.axis), int(current_pair.child)]
        
        # ネクスト情報（簡易版）
        next_colors = [0, 0, 0, 0]  # ダミー（実装簡素化）
        
        # フィールド分析
        analysis = self._analyze_field(field)
        
        return {
            'field_state': field_state,
            'current_colors': current_colors,
            'next_colors': next_colors,
            'turn_count': 0,
            'analysis': analysis
        }
    
    def step(self, action):
        """アクション実行"""
        x, r = action
        
        player = self.game_manager.get_player(0)
        if not player:
            return None, 0, True, {}
        
        # 前の状態を記録
        prev_score = player.get_stats().total_score
        prev_field = player.get_field()
        
        try:
            # ぷよペアの配置をシミュレート
            field = player.get_field()
            
            # 基本的な配置可能性チェック
            if not field.can_place(x, r):
                return self.get_state(), -10, False, {'invalid_move': True}
            
            # 実際の配置処理は複雑なため、簡易的な報酬計算
            reward = self._calculate_reward(field, x, r, prev_score)
            
            # ゲームオーバー判定（簡易版）
            done = self._is_game_over(field)
            
            return self.get_state(), reward, done, {}
            
        except Exception as e:
            return self.get_state(), -5, False, {'error': str(e)}
    
    def get_valid_actions(self):
        """有効なアクションを取得"""
        player = self.game_manager.get_player(0)
        if not player:
            return []
        
        field = player.get_field()
        valid_actions = []
        
        for x in range(6):  # FIELD_WIDTH
            for r in range(4):  # 4つの回転状態
                if field.can_place(x, r):
                    valid_actions.append((x, r))
        
        return valid_actions
    
    def _analyze_field(self, field):
        """フィールド分析"""
        heights = []
        total_puyos = 0
        
        for x in range(6):
            height = 0
            for y in range(13):
                if field.get_puyo(pap.Position(x, y)) != pap.PuyoColor.EMPTY:
                    height = 13 - y
                    break
            heights.append(height)
            total_puyos += height
        
        return {
            'heights': heights,
            'max_height': max(heights),
            'total_puyos': total_puyos,
            'stability': self._calculate_stability(heights)
        }
    
    def _calculate_stability(self, heights):
        """安定性計算"""
        if len(heights) < 2:
            return 0
        
        variance = np.var(heights)
        return max(0, 10 - variance)  # 分散が小さいほど安定
    
    def _calculate_reward(self, field, x, r, prev_score):
        """報酬計算"""
        reward = 0.1  # 基本報酬
        
        # 高さペナルティ
        analysis = self._analyze_field(field)
        if analysis['max_height'] > 10:
            reward -= 5.0
        elif analysis['max_height'] > 8:
            reward -= 1.0
        
        # 中央寄りボーナス
        center_distance = abs(x - 2.5)
        reward += max(0, 2 - center_distance)
        
        # 安定性ボーナス
        reward += analysis['stability'] * 0.1
        
        return reward
    
    def _is_game_over(self, field):
        """ゲームオーバー判定"""
        # 最上段にぷよがあるかチェック
        for x in range(6):
            if field.get_puyo(pap.Position(x, 12)) != pap.PuyoColor.EMPTY:
                return True
        return False


class QLearningAgent:
    """Q-Learning エージェント"""
    
    def __init__(self, config_path="config/ai_params/rl_player.yaml"):
        # YAML設定読み込み
        self.config = self._load_config(config_path)
        
        # Q-table（状態を簡略化して管理）
        self.q_table = {}
        
        # 経験リプレイバッファ
        self.replay_buffer = deque(maxlen=self.config['buffer_size'])
        
        # 学習パラメータ
        self.epsilon = self.config['epsilon_start']
        self.learning_rate = self.config['learning_rate']
        self.discount_factor = self.config['discount_factor']
        
        # 統計
        self.stats = {
            'episode': 0,
            'total_reward': 0,
            'episode_rewards': [],
            'best_reward': float('-inf'),
            'avg_reward_100': 0
        }
    
    def _load_config(self, config_path):
        """設定読み込み"""
        default_config = {
            'learning_rate': 0.1,
            'discount_factor': 0.9,
            'epsilon_start': 1.0,
            'epsilon_end': 0.01,
            'epsilon_decay': 0.995,
            'buffer_size': 10000,
            'batch_size': 32,
            'min_experiences': 100
        }
        
        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                yaml_config = yaml.safe_load(f)
                
            # ネストした設定を平坦化
            config = {}
            if 'learning' in yaml_config:
                config.update(yaml_config['learning'])
            if 'experience_replay' in yaml_config:
                config.update(yaml_config['experience_replay'])
            
            # デフォルト値で補完
            for key, value in default_config.items():
                if key not in config:
                    config[key] = value
                    
            return config
            
        except Exception as e:
            print(f"YAML設定読み込みエラー（デフォルト設定使用）: {e}")
            return default_config
    
    def _state_to_key(self, state):
        """状態をQ-tableキーに変換（簡略化）"""
        heights = state['analysis']['heights']
        colors = tuple(state['current_colors'])
        height_profile = tuple(min(h, 10) for h in heights)  # 高さを10で制限
        return f"{height_profile}_{colors}"
    
    def choose_action(self, state, valid_actions):
        """行動選択（ε-greedy）"""
        if not valid_actions:
            return None
        
        # ε-greedy戦略
        if random.random() < self.epsilon:
            return random.choice(valid_actions)
        
        # Q値に基づく選択
        state_key = self._state_to_key(state)
        if state_key not in self.q_table:
            return random.choice(valid_actions)
        
        best_action = None
        best_q_value = float('-inf')
        
        for action in valid_actions:
            action_key = f"{action[0]}_{action[1]}"
            q_value = self.q_table[state_key].get(action_key, 0.0)
            
            if q_value > best_q_value:
                best_q_value = q_value
                best_action = action
        
        return best_action if best_action else random.choice(valid_actions)
    
    def learn(self, state, action, reward, next_state, done):
        """Q値更新"""
        state_key = self._state_to_key(state)
        action_key = f"{action[0]}_{action[1]}"
        
        # Q-tableエントリ初期化
        if state_key not in self.q_table:
            self.q_table[state_key] = {}
        
        current_q = self.q_table[state_key].get(action_key, 0.0)
        
        # 次状態の最大Q値
        max_next_q = 0.0
        if not done and next_state:
            next_state_key = self._state_to_key(next_state)
            if next_state_key in self.q_table:
                max_next_q = max(self.q_table[next_state_key].values(), default=0.0)
        
        # Q値更新
        target_q = reward + self.discount_factor * max_next_q
        updated_q = current_q + self.learning_rate * (target_q - current_q)
        
        self.q_table[state_key][action_key] = updated_q
        
        # 経験をバッファに追加
        experience = {
            'state': state,
            'action': action,
            'reward': reward,
            'next_state': next_state,
            'done': done
        }
        self.replay_buffer.append(experience)
    
    def decay_epsilon(self):
        """ε値の減衰"""
        self.epsilon = max(self.config['epsilon_end'], 
                          self.epsilon * self.config['epsilon_decay'])
    
    def update_stats(self, episode_reward):
        """統計更新"""
        self.stats['episode'] += 1
        self.stats['total_reward'] += episode_reward
        self.stats['episode_rewards'].append(episode_reward)
        
        if episode_reward > self.stats['best_reward']:
            self.stats['best_reward'] = episode_reward
        
        # 直近100エピソードの平均
        recent_rewards = self.stats['episode_rewards'][-100:]
        self.stats['avg_reward_100'] = np.mean(recent_rewards)
    
    def save_model(self, filepath):
        """モデル保存"""
        model_data = {
            'q_table': self.q_table,
            'stats': self.stats,
            'config': self.config,
            'epsilon': self.epsilon,
            'timestamp': datetime.now().isoformat()
        }
        
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        
        with open(filepath, 'wb') as f:
            pickle.dump(model_data, f)
    
    def load_model(self, filepath):
        """モデル読み込み"""
        try:
            with open(filepath, 'rb') as f:
                model_data = pickle.load(f)
                
            self.q_table = model_data['q_table']
            self.stats = model_data['stats']
            self.epsilon = model_data.get('epsilon', self.config['epsilon_end'])
            
            print(f"モデル読み込み成功: {filepath}")
            print(f"Q-table サイズ: {len(self.q_table)}")
            print(f"Best reward: {self.stats['best_reward']:.2f}")
            
            return True
            
        except Exception as e:
            print(f"モデル読み込み失敗: {e}")
            return False


class RLTrainer:
    """RL訓練管理システム"""
    
    def __init__(self, config_path="config/ai_params/rl_player.yaml"):
        self.env = RLTrainingEnvironment()
        self.agent = QLearningAgent(config_path)
        self.config_path = config_path
        
        # 訓練設定
        self.max_episodes = 1000
        self.max_steps_per_episode = 100
        self.save_interval = 50
        self.eval_interval = 100
        
        # 結果保存
        self.results_dir = "training_results"
        os.makedirs(self.results_dir, exist_ok=True)
    
    def train(self, episodes=None, resume=False):
        """訓練実行"""
        if episodes is None:
            episodes = self.max_episodes
        
        print(f"=== RLPlayerAI 事前学習開始 ===")
        print(f"エピソード数: {episodes}")
        print(f"設定ファイル: {self.config_path}")
        print(f"結果保存先: {self.results_dir}")
        
        # モデル読み込み（resume=True時）
        if resume:
            model_path = os.path.join(self.results_dir, "rl_model.pkl")
            self.agent.load_model(model_path)
        
        start_episode = self.agent.stats['episode']
        
        for episode in range(start_episode, start_episode + episodes):
            episode_reward = self._train_episode()
            self.agent.update_stats(episode_reward)
            self.agent.decay_epsilon()
            
            # 進捗表示
            if (episode + 1) % 10 == 0:
                stats = self.agent.stats
                print(f"Episode {episode + 1}: "
                      f"Reward={episode_reward:.2f}, "
                      f"Avg100={stats['avg_reward_100']:.2f}, "
                      f"Best={stats['best_reward']:.2f}, "
                      f"ε={self.agent.epsilon:.3f}")
            
            # モデル保存
            if (episode + 1) % self.save_interval == 0:
                model_path = os.path.join(self.results_dir, f"rl_model_ep{episode+1}.pkl")
                self.agent.save_model(model_path)
                
                # ベストモデル更新
                if episode_reward >= self.agent.stats['best_reward']:
                    best_path = os.path.join(self.results_dir, "rl_model_best.pkl")
                    self.agent.save_model(best_path)
        
        # 最終モデル保存
        final_path = os.path.join(self.results_dir, "rl_model.pkl")
        self.agent.save_model(final_path)
        
        # 訓練結果サマリー
        self._save_training_summary()
        
        print(f"\n=== 訓練完了 ===")
        print(f"総エピソード: {self.agent.stats['episode']}")
        print(f"最高報酬: {self.agent.stats['best_reward']:.2f}")
        print(f"平均報酬(最新100): {self.agent.stats['avg_reward_100']:.2f}")
        print(f"モデル保存: {final_path}")
        
        return self.agent.stats
    
    def _train_episode(self):
        """1エピソードの訓練"""
        state = self.env.reset()
        total_reward = 0
        steps = 0
        
        for _ in range(self.max_steps_per_episode):
            valid_actions = self.env.get_valid_actions()
            if not valid_actions:
                break
            
            action = self.agent.choose_action(state, valid_actions)
            if action is None:
                break
            
            next_state, reward, done, info = self.env.step(action)
            
            if next_state is not None:
                self.agent.learn(state, action, reward, next_state, done)
                state = next_state
                total_reward += reward
                steps += 1
                
                if done:
                    break
            else:
                break
        
        return total_reward
    
    def _save_training_summary(self):
        """訓練結果保存"""
        summary = {
            'config': self.agent.config,
            'stats': self.agent.stats,
            'q_table_size': len(self.agent.q_table),
            'final_epsilon': self.agent.epsilon,
            'training_completed': datetime.now().isoformat()
        }
        
        summary_path = os.path.join(self.results_dir, "training_summary.json")
        with open(summary_path, 'w', encoding='utf-8') as f:
            json.dump(summary, f, indent=2, ensure_ascii=False)
        
        print(f"訓練サマリー保存: {summary_path}")
    
    def evaluate(self, episodes=10):
        """評価実行"""
        print(f"=== モデル評価 ({episodes}エピソード) ===")
        
        # 評価時はε=0（最適行動のみ）
        original_epsilon = self.agent.epsilon
        self.agent.epsilon = 0.0
        
        rewards = []
        for episode in range(episodes):
            episode_reward = self._train_episode()
            rewards.append(episode_reward)
            print(f"Eval Episode {episode + 1}: Reward={episode_reward:.2f}")
        
        # ε値復元
        self.agent.epsilon = original_epsilon
        
        avg_reward = np.mean(rewards)
        std_reward = np.std(rewards)
        
        print(f"評価結果: 平均報酬={avg_reward:.2f}±{std_reward:.2f}")
        return avg_reward, std_reward


def main():
    """メイン訓練プログラム"""
    import argparse
    
    parser = argparse.ArgumentParser(description='RLPlayerAI事前学習')
    parser.add_argument('--episodes', type=int, default=500, help='訓練エピソード数')
    parser.add_argument('--resume', action='store_true', help='訓練を再開')
    parser.add_argument('--eval', action='store_true', help='評価のみ実行')
    parser.add_argument('--config', default='config/ai_params/rl_player.yaml', help='設定ファイル')
    
    args = parser.parse_args()
    
    try:
        trainer = RLTrainer(args.config)
        
        if args.eval:
            # 評価のみ
            model_path = "training_results/rl_model.pkl"
            if trainer.agent.load_model(model_path):
                trainer.evaluate()
            else:
                print("評価用モデルが見つかりません")
        else:
            # 訓練実行
            stats = trainer.train(episodes=args.episodes, resume=args.resume)
            
            # 訓練後評価
            print("\n=== 訓練後評価 ===")
            trainer.evaluate()
        
    except KeyboardInterrupt:
        print("\n訓練が中断されました")
    except Exception as e:
        print(f"エラー: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()