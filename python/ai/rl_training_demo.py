#!/usr/bin/env python3
"""
RLPlayerAI事前学習システム（簡易デモ版）
実際のぷよぷよAPIの制約を考慮したデモ実装
"""

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

import numpy as np
import random
import json
import yaml
from collections import deque
import time
from datetime import datetime
import pickle


class SimplePuyoEnvironment:
    """簡易ぷよぷよ環境（学習デモ用）"""
    
    def __init__(self):
        self.width = 6
        self.height = 13
        self.reset()
    
    def reset(self):
        """環境のリセット"""
        self.field = np.zeros((self.height, self.width), dtype=int)
        self.current_pair = self._generate_pair()
        self.score = 0
        self.turns = 0
        return self.get_state()
    
    def get_state(self):
        """現在の状態を取得"""
        # フィールド状態を平坦化
        field_state = self.field.flatten().tolist()
        
        # 各列の高さを計算
        heights = []
        for x in range(self.width):
            height = 0
            for y in range(self.height):
                if self.field[y, x] != 0:
                    height = self.height - y
                    break
            heights.append(height)
        
        return {
            'field_state': field_state,
            'current_colors': self.current_pair,
            'next_colors': [0, 0, 0, 0],  # ダミー
            'turn_count': self.turns,
            'analysis': {
                'heights': heights,
                'max_height': max(heights),
                'total_puyos': np.sum(self.field > 0),
                'stability': self._calculate_stability(heights)
            }
        }
    
    def get_valid_actions(self):
        """有効なアクションを取得"""
        valid_actions = []
        
        for x in range(self.width):
            for r in range(4):  # 回転状態
                if self._can_place(x, r):
                    valid_actions.append((x, r))
        
        return valid_actions
    
    def step(self, action):
        """アクション実行"""
        x, r = action
        
        if not self._can_place(x, r):
            return self.get_state(), -10, False, {'invalid_move': True}
        
        # ぷよ配置のシミュレート
        reward = self._place_puyo(x, r)
        
        # 新しいペア生成
        self.current_pair = self._generate_pair()
        self.turns += 1
        
        # ゲームオーバー判定
        done = self._is_game_over()
        
        return self.get_state(), reward, done, {}
    
    def _generate_pair(self):
        """ランダムなぷよペア生成"""
        return [random.randint(1, 4), random.randint(1, 4)]
    
    def _can_place(self, x, r):
        """配置可能判定"""
        # 簡易判定：上から2段目までに空きがあるか
        return x >= 0 and x < self.width and self.field[1, x] == 0
    
    def _place_puyo(self, x, r):
        """ぷよ配置と報酬計算"""
        # 配置位置を決定（最下段から）
        y = self.height - 1
        for row in range(self.height - 1, -1, -1):
            if self.field[row, x] == 0:
                y = row
                break
        
        # ぷよ配置
        if y > 0:
            self.field[y, x] = self.current_pair[0]      # 軸ぷよ
            self.field[y-1, x] = self.current_pair[1]    # 子ぷよ
        else:
            self.field[y, x] = self.current_pair[0]
        
        # 簡易連鎖判定と報酬計算
        chains = self._check_chains()
        reward = self._calculate_reward(x, chains)
        
        return reward
    
    def _check_chains(self):
        """簡易連鎖判定"""
        chains = 0
        # 4個以上の同色隣接をチェック（簡易版）
        for color in range(1, 5):
            if self._count_connected(color) >= 4:
                chains += 1
        
        return chains
    
    def _count_connected(self, color):
        """特定色の最大連結数"""
        max_connected = 0
        visited = np.zeros_like(self.field, dtype=bool)
        
        for y in range(self.height):
            for x in range(self.width):
                if not visited[y, x] and self.field[y, x] == color:
                    connected = self._dfs_count(x, y, color, visited)
                    max_connected = max(max_connected, connected)
        
        return max_connected
    
    def _dfs_count(self, x, y, color, visited):
        """DFSで連結数を計算"""
        if (x < 0 or x >= self.width or y < 0 or y >= self.height or
            visited[y, x] or self.field[y, x] != color):
            return 0
        
        visited[y, x] = True
        count = 1
        
        # 4方向探索
        for dx, dy in [(0, 1), (0, -1), (1, 0), (-1, 0)]:
            count += self._dfs_count(x + dx, y + dy, color, visited)
        
        return count
    
    def _calculate_reward(self, x, chains):
        """報酬計算"""
        reward = 0.1  # 基本報酬
        
        # 連鎖ボーナス
        if chains > 0:
            reward += chains * 10.0
        
        # 中央寄りボーナス
        center_distance = abs(x - 2.5)
        reward += max(0, 2 - center_distance)
        
        # 高さペナルティ
        heights = [np.sum(self.field[:, col] > 0) for col in range(self.width)]
        max_height = max(heights) if heights else 0
        if max_height > 10:
            reward -= 5.0
        
        return reward
    
    def _calculate_stability(self, heights):
        """安定性計算"""
        if len(heights) < 2:
            return 0
        
        variance = np.var(heights)
        return max(0, 10 - variance)
    
    def _is_game_over(self):
        """ゲームオーバー判定"""
        # 最上段にぷよがあるか
        return np.any(self.field[0, :] > 0)


class DemoQLearningAgent:
    """デモ用Q-Learning エージェント"""
    
    def __init__(self, config_path="config/ai_params/rl_player.yaml"):
        # 基本設定
        self.config = {
            'learning_rate': 0.1,
            'discount_factor': 0.9,
            'epsilon_start': 1.0,
            'epsilon_end': 0.01,
            'epsilon_decay': 0.995,
            'buffer_size': 1000
        }
        
        self._try_load_config(config_path)
        
        # Q-table
        self.q_table = {}
        
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
    
    def _try_load_config(self, config_path):
        """設定読み込み試行"""
        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                yaml_config = yaml.safe_load(f)
                
            if 'learning' in yaml_config:
                learning_config = yaml_config['learning']
                self.config.update({
                    'learning_rate': learning_config.get('learning_rate', 0.1),
                    'discount_factor': learning_config.get('discount_factor', 0.9),
                    'epsilon_start': learning_config.get('epsilon_start', 1.0),
                    'epsilon_end': learning_config.get('epsilon_end', 0.01),
                    'epsilon_decay': learning_config.get('epsilon_decay', 0.995)
                })
                
        except Exception as e:
            print(f"設定ファイル読み込みエラー（デフォルト使用）: {e}")
    
    def _state_to_key(self, state):
        """状態をキーに変換"""
        heights = tuple(min(h, 12) for h in state['analysis']['heights'])
        colors = tuple(state['current_colors'])
        return f"{heights}_{colors}"
    
    def choose_action(self, state, valid_actions):
        """行動選択"""
        if not valid_actions or random.random() < self.epsilon:
            return random.choice(valid_actions) if valid_actions else None
        
        # Q値に基づく選択
        state_key = self._state_to_key(state)
        if state_key not in self.q_table:
            return random.choice(valid_actions)
        
        best_action = None
        best_q = float('-inf')
        
        for action in valid_actions:
            action_key = f"{action[0]}_{action[1]}"
            q_value = self.q_table[state_key].get(action_key, 0.0)
            
            if q_value > best_q:
                best_q = q_value
                best_action = action
        
        return best_action if best_action else random.choice(valid_actions)
    
    def learn(self, state, action, reward, next_state, done):
        """Q値更新"""
        state_key = self._state_to_key(state)
        action_key = f"{action[0]}_{action[1]}"
        
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
    
    def decay_epsilon(self):
        """ε値減衰"""
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


def demo_training(episodes=50):
    """デモ学習実行"""
    print(f"=== RLPlayerAI事前学習システム（デモ版） ===")
    print(f"エピソード数: {episodes}")
    print(f"環境: 簡易ぷよぷよシミュレータ")
    
    env = SimplePuyoEnvironment()
    agent = DemoQLearningAgent()
    
    for episode in range(episodes):
        state = env.reset()
        episode_reward = 0
        steps = 0
        max_steps = 50
        
        for step in range(max_steps):
            valid_actions = env.get_valid_actions()
            if not valid_actions:
                break
            
            action = agent.choose_action(state, valid_actions)
            if action is None:
                break
            
            next_state, reward, done, info = env.step(action)
            
            agent.learn(state, action, reward, next_state, done)
            
            state = next_state
            episode_reward += reward
            steps += 1
            
            if done:
                break
        
        agent.update_stats(episode_reward)
        agent.decay_epsilon()
        
        # 進捗表示
        if (episode + 1) % 10 == 0 or episode == 0:
            print(f"Episode {episode + 1:3d}: "
                  f"Reward={episode_reward:6.2f}, "
                  f"Steps={steps:2d}, "
                  f"Best={agent.stats['best_reward']:6.2f}, "
                  f"ε={agent.epsilon:.3f}")
    
    # 結果保存
    os.makedirs("training_results", exist_ok=True)
    
    # 学習結果保存
    model_data = {
        'q_table': agent.q_table,
        'stats': agent.stats,
        'config': agent.config,
        'epsilon': agent.epsilon,
        'timestamp': datetime.now().isoformat()
    }
    
    model_path = "training_results/rl_demo_model.pkl"
    with open(model_path, 'wb') as f:
        pickle.dump(model_data, f)
    
    # サマリー保存
    summary = {
        'episodes': episodes,
        'final_stats': agent.stats,
        'q_table_size': len(agent.q_table),
        'demo_completed': datetime.now().isoformat()
    }
    
    summary_path = "training_results/demo_summary.json"
    with open(summary_path, 'w', encoding='utf-8') as f:
        json.dump(summary, f, indent=2, ensure_ascii=False)
    
    print(f"\n=== デモ学習完了 ===")
    print(f"総エピソード: {agent.stats['episode']}")
    print(f"最高報酬: {agent.stats['best_reward']:.2f}")
    print(f"平均報酬: {agent.stats['avg_reward_100']:.2f}")
    print(f"Q-table サイズ: {len(agent.q_table)}")
    print(f"デモモデル保存: {model_path}")
    print(f"サマリー保存: {summary_path}")
    
    return agent.stats


def main():
    """メイン実行"""
    import argparse
    
    parser = argparse.ArgumentParser(description='RLPlayerAI事前学習デモ')
    parser.add_argument('--episodes', type=int, default=50, help='学習エピソード数')
    
    args = parser.parse_args()
    
    try:
        demo_training(args.episodes)
    except KeyboardInterrupt:
        print("\n学習が中断されました")
    except Exception as e:
        print(f"エラー: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()