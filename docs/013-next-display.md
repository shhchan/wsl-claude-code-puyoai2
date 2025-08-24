# チケット013: ネクストぷよ表示実装

## 概要
C++のNextGeneratorクラスをPython UIで活用し、ネクストぷよの表示機能を実装

## 現状分析

### 実装されているもの
- ✅ C++のNextGeneratorクラス（3つ先までのNEXTペア生成）
- ✅ PlayerクラスにNextGeneratorのアクセサ
- ⚠️ Python bindingsではNextGeneratorが未露出

### 実装されていないもの
- ❌ NextGeneratorのPython露出
- ❌ UIでのネクスト表示領域
- ❌ GameControllerでのNextGenerator活用
- ❌ 固定ペア生成からNextGeneratorへの切替

## 詳細タスク

### 1. NextGeneratorのPython連携
- [x] `python_bindings.cpp`にNextGeneratorクラス追加
- [x] `set_active_colors`, `get_current_pair`, `get_next_pair`, `advance_to_next`メソッド露出
- [x] PlayerクラスからのNextGeneratorアクセサ追加

### 2. GameControllerの改修
- [x] 固定ペア生成の`_generate_new_pair()`を削除
- [x] `Player.get_next_generator().get_current_pair()`からペア取得に変更
- [x] ペア設置後に`advance_to_next()`を呼び出すよう変更

### 3. UI表示機能実装
- [x] `renderer.py`にネクスト表示エリアを追加
- [x] 現在・1つ目・2つ目のネクストペア表示
- [x] ネクスト表示領域のレイアウト調整（右側UIエリア活用）

### 4. 統合テスト
- [x] NextGeneratorからの正しいペア取得確認
- [x] ネクスト表示の視覚的確認
- [x] 既存UI機能の動作確認

## 技術詳細

### C++側既存機能
```cpp
class NextGenerator {
    PuyoPair get_current_pair() const;        // 現在のペア
    PuyoPair get_next_pair(int index) const;  // 0=現在, 1=次, 2=その次
    void advance_to_next();                   // 次のペアに進む
    void set_active_colors(const std::vector<PuyoColor>& colors);
};

class Player {
    NextGenerator& get_next_generator();      // NextGeneratorアクセス
};
```

### Python連携目標
```python
# NextGeneratorアクセス
player = game_manager.get_player(0)
next_gen = player.get_next_generator()

# 現在とネクストペアの取得
current_pair = next_gen.get_current_pair()
next1_pair = next_gen.get_next_pair(1)
next2_pair = next_gen.get_next_pair(2)

# ペア使用後の進行
next_gen.advance_to_next()
```

## 依存関係
- チケット006（Python連携）完了後に開始
- チケット007（Python UI実装）完了後に開始

## 優先度
高（人間プレイヤー機能の基本要素）

## 推定工数
2-3時間

## 完了条件
- ✅ NextGeneratorがPythonから利用できること
- ✅ ランダム生成されたネクストぷよが画面右側に表示されること
- ✅ 現在・1つ先・2つ先の合計3つのペアが確認できること
- ✅ ペア設置後に次のペアが正しく順送りされること
- ✅ 既存のゲーム機能が正常動作すること

## 実装結果

### 実装ファイル
- `cpp/bindings/python_bindings.cpp`: NextGeneratorクラスとPlayer.get_next_generator()の露出
- `python/ui/game_controller.py`: NextGenerator統合とペア管理改修
- `python/ui/renderer.py`: ネクスト表示エリアとプレビュー機能追加
- `docs/test-results-013.md`: 詳細なテスト結果記録

### 主要成果
- ✅ NextGeneratorのPython完全統合
- ✅ ランダムペア生成による固定ペアからの脱却
- ✅ Current/Next1/Next2の3段階ネクスト表示実装
- ✅ 既存UI機能との完全統合（60FPS安定動作維持）
- ✅ 全テスト項目クリア

**チケット013完了 (2025-08-24)**