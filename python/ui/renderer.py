#!/usr/bin/env python3
"""
ゲーム描画システム
pygameを使用したぷよぷよのビジュアル描画
"""

import pygame
import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

import puyo_ai_platform as pap

# 色定義
COLORS = {
    pap.PuyoColor.EMPTY: (240, 240, 240),      # ライトグレー
    pap.PuyoColor.RED: (255, 100, 100),        # 赤
    pap.PuyoColor.GREEN: (100, 255, 100),      # 緑
    pap.PuyoColor.BLUE: (100, 100, 255),       # 青
    pap.PuyoColor.YELLOW: (255, 255, 100),     # 黄
    pap.PuyoColor.PURPLE: (255, 100, 255),     # 紫
    pap.PuyoColor.GARBAGE: (128, 128, 128),    # グレー（おじゃま）
}

# UIレイアウト定数
CELL_SIZE = 40          # 各セルのピクセルサイズ
FIELD_MARGIN = 20       # フィールド周りの余白
UI_WIDTH = 300          # UI部分の幅
WINDOW_WIDTH = FIELD_MARGIN * 2 + pap.FIELD_WIDTH * CELL_SIZE + UI_WIDTH
WINDOW_HEIGHT = FIELD_MARGIN * 2 + pap.FIELD_HEIGHT * CELL_SIZE

class PuyoRenderer:
    """ぷよぷよゲームの描画クラス"""
    
    def __init__(self):
        pygame.init()
        self.screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
        pygame.display.set_caption("Puyo Puyo AI Platform")
        self.clock = pygame.time.Clock()
        
        # フォント初期化
        self.font = pygame.font.Font(None, 24)
        self.large_font = pygame.font.Font(None, 36)
        
        # フィールド描画位置
        self.field_x = FIELD_MARGIN
        self.field_y = FIELD_MARGIN
        
        # UI描画位置
        self.ui_x = FIELD_MARGIN * 2 + pap.FIELD_WIDTH * CELL_SIZE
        
    def clear_screen(self):
        """画面クリア"""
        self.screen.fill((50, 50, 50))  # ダークグレー背景
    
    def draw_cell(self, x, y, color, border=True):
        """単一セルの描画"""
        pixel_x = self.field_x + x * CELL_SIZE
        pixel_y = self.field_y + (pap.FIELD_HEIGHT - 1 - y) * CELL_SIZE
        
        rect = pygame.Rect(pixel_x, pixel_y, CELL_SIZE, CELL_SIZE)
        pygame.draw.rect(self.screen, COLORS[color], rect)
        
        if border:
            pygame.draw.rect(self.screen, (0, 0, 0), rect, 2)
    
    def draw_field(self, field):
        """フィールド全体の描画"""
        # フィールド背景
        field_rect = pygame.Rect(
            self.field_x - 2, self.field_y - 2,
            pap.FIELD_WIDTH * CELL_SIZE + 4,
            pap.FIELD_HEIGHT * CELL_SIZE + 4
        )
        pygame.draw.rect(self.screen, (0, 0, 0), field_rect, 2)
        
        # 可視部分との境界線を表示（12段目と13段目の間）
        boundary_y = self.field_y + (pap.FIELD_HEIGHT - pap.VISIBLE_HEIGHT) * CELL_SIZE - 1
        pygame.draw.line(self.screen, (255, 255, 0), 
                        (self.field_x, boundary_y), 
                        (self.field_x + pap.FIELD_WIDTH * CELL_SIZE, boundary_y), 2)
        
        # 各セルを描画（14段全体）
        for y in range(pap.FIELD_HEIGHT):
            for x in range(pap.FIELD_WIDTH):
                pos = pap.Position(x, y)
                color = field.get_puyo(pos)
                # 隠し段（13段目以上）は少し暗く表示
                if y >= pap.VISIBLE_HEIGHT:
                    if color != pap.PuyoColor.EMPTY:
                        # 隠し段のぷよは少し暗く表示
                        dark_color = tuple(max(0, c - 50) for c in COLORS[color])
                        pixel_x = self.field_x + x * CELL_SIZE
                        pixel_y = self.field_y + (pap.FIELD_HEIGHT - 1 - y) * CELL_SIZE
                        rect = pygame.Rect(pixel_x, pixel_y, CELL_SIZE, CELL_SIZE)
                        pygame.draw.rect(self.screen, dark_color, rect)
                        pygame.draw.rect(self.screen, (100, 100, 100), rect, 2)
                    else:
                        # 空セルは格子のみ表示
                        pixel_x = self.field_x + x * CELL_SIZE
                        pixel_y = self.field_y + (pap.FIELD_HEIGHT - 1 - y) * CELL_SIZE
                        rect = pygame.Rect(pixel_x, pixel_y, CELL_SIZE, CELL_SIZE)
                        pygame.draw.rect(self.screen, (80, 80, 80), rect, 1)
                else:
                    self.draw_cell(x, y, color)
    
    def draw_puyo_pair(self, pair, highlight=False):
        """操作中のぷよペアを描画（14段全体対応）"""
        # 軸ぷよ
        if pair.pos.y < pap.FIELD_HEIGHT:
            color = COLORS[pair.axis]
            if highlight:
                # ハイライト効果
                color = tuple(min(255, c + 50) for c in color)
            
            # 隠し段の場合は少し暗くする
            if pair.pos.y >= pap.VISIBLE_HEIGHT:
                color = tuple(max(0, c - 30) for c in color)
            
            pixel_x = self.field_x + pair.pos.x * CELL_SIZE
            pixel_y = self.field_y + (pap.FIELD_HEIGHT - 1 - pair.pos.y) * CELL_SIZE
            rect = pygame.Rect(pixel_x, pixel_y, CELL_SIZE, CELL_SIZE)
            pygame.draw.rect(self.screen, color, rect)
            pygame.draw.rect(self.screen, (255, 255, 255), rect, 3)  # 白い枠
        
        # 子ぷよ
        child_pos = pair.get_child_position()
        if child_pos.y < pap.FIELD_HEIGHT:
            color = COLORS[pair.child]
            if highlight:
                color = tuple(min(255, c + 50) for c in color)
            
            # 隠し段の場合は少し暗くする
            if child_pos.y >= pap.VISIBLE_HEIGHT:
                color = tuple(max(0, c - 30) for c in color)
            
            pixel_x = self.field_x + child_pos.x * CELL_SIZE
            pixel_y = self.field_y + (pap.FIELD_HEIGHT - 1 - child_pos.y) * CELL_SIZE
            rect = pygame.Rect(pixel_x, pixel_y, CELL_SIZE, CELL_SIZE)
            pygame.draw.rect(self.screen, color, rect)
            pygame.draw.rect(self.screen, (255, 255, 255), rect, 3)  # 白い枠
    
    def draw_text(self, text, x, y, color=(255, 255, 255), large=False):
        """テキスト描画"""
        font = self.large_font if large else self.font
        surface = font.render(text, True, color)
        self.screen.blit(surface, (x, y))
        return surface.get_height()
    
    def draw_next_display(self, next_generator, y_start):
        """ネクスト表示"""
        if not next_generator:
            return y_start
        
        x = self.ui_x + 10
        y = y_start
        
        # ネクストヘッダー
        y += self.draw_text("NEXT", x, y, large=True) + 10
        
        # 現在操作中（参考表示）
        try:
            current_pair = next_generator.get_current_pair()
            y += self.draw_text("Current:", x, y) + 5
            self.draw_next_pair_preview(current_pair, x + 10, y, small=True)
            y += 35
        except:
            pass
        
        # ネクストペア表示（1つ先、2つ先）
        for i in range(1, 3):
            try:
                next_pair = next_generator.get_next_pair(i)
                y += self.draw_text(f"Next {i}:", x, y) + 5
                self.draw_next_pair_preview(next_pair, x + 10, y, small=True)
                y += 35
            except:
                pass
        
        return y + 10
    
    def draw_next_pair_preview(self, pair, x, y, small=True):
        """ネクストペアのプレビュー描画"""
        size = 20 if small else CELL_SIZE
        
        # 軸ぷよ（左）
        rect1 = pygame.Rect(x, y, size, size)
        pygame.draw.rect(self.screen, COLORS[pair.axis], rect1)
        pygame.draw.rect(self.screen, (0, 0, 0), rect1, 1)
        
        # 子ぷよ（右）
        rect2 = pygame.Rect(x + size + 2, y, size, size)
        pygame.draw.rect(self.screen, COLORS[pair.child], rect2)
        pygame.draw.rect(self.screen, (0, 0, 0), rect2, 1)
    
    def draw_player_info(self, player, y_offset=0):
        """プレイヤー情報描画"""
        x = self.ui_x + 10
        y = 20 + y_offset
        
        # プレイヤー名
        y += self.draw_text(f"Player: {player.get_name()}", x, y, large=True) + 5
        
        # ネクスト表示
        next_generator = player.get_next_generator()
        y = self.draw_next_display(next_generator, y + 10)
        
        # 統計情報
        stats = player.get_stats()
        y += self.draw_text(f"Score: {stats.total_score}", x, y) + 5
        y += self.draw_text(f"Max Chain: {stats.max_chain}", x, y) + 5
        y += self.draw_text(f"Total Chains: {stats.total_chains}", x, y) + 5
        y += self.draw_text(f"Sent Garbage: {stats.sent_garbage}", x, y) + 5
        y += self.draw_text(f"Recv Garbage: {stats.received_garbage}", x, y) + 5
        
        # ゲーム状態
        state_text = f"State: {player.get_state()}"
        y += self.draw_text(state_text, x, y) + 10
        
        return y
    
    def draw_game_info(self, game_manager):
        """ゲーム全体情報描画"""
        x = self.ui_x + 10
        y = 300
        
        y += self.draw_text(f"Mode: {game_manager.get_mode()}", x, y) + 5
        y += self.draw_text(f"State: {game_manager.get_state()}", x, y) + 5
        y += self.draw_text(f"Step: {game_manager.get_current_step()}", x, y) + 5
        y += self.draw_text(f"Turn: {game_manager.get_turn_count()}", x, y) + 5
        
        # 現在のプレイヤー
        current_player = game_manager.get_current_player()
        y += self.draw_text(f"Current Player: {current_player}", x, y) + 10
        
        return y
    
    def draw_controls_help(self):
        """操作方法のヘルプ描画"""
        x = self.ui_x + 10
        y = WINDOW_HEIGHT - 150
        
        self.draw_text("Controls:", x, y, color=(200, 200, 200))
        y += 25
        self.draw_text("A/D - Move Left/Right", x, y)
        y += 20
        self.draw_text("↓/→ - Rotate CCW/CW", x, y)
        y += 20
        self.draw_text("W - Drop", x, y)
        y += 20
        self.draw_text("R - Reset Game", x, y)
        y += 20
        self.draw_text("ESC - Quit", x, y)
    
    def update_display(self):
        """画面更新"""
        pygame.display.flip()
        self.clock.tick(60)  # 60 FPS
    
    def get_events(self):
        """イベント取得"""
        return pygame.event.get()
    
    def quit(self):
        """終了処理"""
        pygame.quit()

class GameVisualizer:
    """ゲーム全体の可視化管理クラス"""
    
    def __init__(self):
        self.renderer = PuyoRenderer()
        self.running = True
    
    def render_game(self, game_manager, current_pair=None, highlight_pair=False):
        """ゲーム全体の描画"""
        self.renderer.clear_screen()
        
        # プレイヤー情報描画
        y_offset = 0
        for i in range(2):  # 最大2人まで
            player = game_manager.get_player(i)
            if player:
                y_offset = self.renderer.draw_player_info(player, y_offset) + 20
        
        # ゲーム情報描画
        self.renderer.draw_game_info(game_manager)
        
        # 操作ヘルプ
        self.renderer.draw_controls_help()
        
        # フィールド描画（現在のプレイヤー）
        current_player_id = game_manager.get_current_player()
        current_player = game_manager.get_player(current_player_id)
        if current_player:
            field = current_player.get_field()
            self.renderer.draw_field(field)
            
            # 操作中のぷよペア描画
            if current_pair:
                self.renderer.draw_puyo_pair(current_pair, highlight_pair)
        
        self.renderer.update_display()
    
    def handle_events(self):
        """イベント処理"""
        for event in self.renderer.get_events():
            if event.type == pygame.QUIT:
                self.running = False
                return None
            elif event.type == pygame.KEYDOWN:
                return event.key
        return None
    
    def is_running(self):
        """実行中かチェック"""
        return self.running
    
    def quit(self):
        """終了"""
        self.renderer.quit()
        self.running = False