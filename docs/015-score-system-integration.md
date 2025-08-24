# チケット015: スコアシステム統合実装

## 概要
C++のScoreCalculatorをPython UIで活用し、正確なスコア計算とスコア表示機能を実装

## 現状分析

### 実装されているもの
- ✅ C++のScoreCalculatorクラス（連鎖・落下・全消しボーナス計算）
- ✅ ChainSystemでのScoreCalculator統合
- ⚠️ Python bindingsではScoreCalculator関連が未露出
- ⚠️ 現在のスコア表示は初期値0のまま更新されない

### 実装されていないもの
- ❌ ScoreCalculator関連のPython露出
- ❌ 連鎖後のスコア自動計算・更新
- ❌ UIでの正確なスコア表示
- ❌ 落下ボーナスの適用

## 詳細タスク

### 1. ScoreCalculatorのPython連携
- [ ] `python_bindings.cpp`にScoreResult構造体追加
- [ ] `python_bindings.cpp`にScoreCalculatorクラス追加（チケット014と連携）
- [ ] ChainSystemResultからのスコア結果取得
- [ ] PlayerStatsのスコア更新メソッド露出

### 2. スコア計算ロジック統合
- [ ] チケット014の連鎖実行と統合してスコア自動計算
- [ ] 落下ボーナスの適切な適用（ドロップ高さ計算）
- [ ] PlayerStatsへのスコア累積処理
- [ ] 全消しボーナスの検出・適用

### 3. UI表示機能実装
- [ ] `renderer.py`のスコア表示を動的更新に変更
- [ ] 獲得スコア詳細の表示（連鎖ボーナス、落下ボーナス等）
- [ ] スコア増加時の視覚的エフェクト（基本的なもの）

### 4. 統合テスト
- [ ] 連鎖時のスコア正確性確認
- [ ] 落下ボーナスの正確性確認
- [ ] 全消しボーナスの動作確認
- [ ] 累積スコアの表示確認

## 技術詳細

### C++側既存機能
```cpp
struct ScoreResult {
    int chain_score;      // 連鎖スコア
    int drop_bonus;       // 落下ボーナス
    int all_clear_bonus;  // 全消しボーナス
    int total_score;      // 合計スコア
};

class ScoreCalculator {
    ScoreResult calculate_chain_score(const ChainResult& result);
    ScoreResult calculate_drop_bonus(int drop_height);
    void set_all_clear_mode(bool enabled);
};

struct ChainSystemResult {
    ScoreResult score_result;  // スコア計算結果含む
};
```

### Python統合目標
```python
# チケット014の連鎖実行と連携
chain_result = chain_system.execute_chains_with_drop_bonus(drop_height)

if chain_result.has_chains():
    # スコア結果取得
    score_result = chain_result.score_result
    
    # 統計更新
    stats = player.get_stats()
    stats.total_score += score_result.total_score
    
    # UI表示更新
```

## UI実装方針

### スコア表示位置
- 右側UIエリアの中央部（プレイヤー統計部分）
- 現在のスコア: 大きなフォント
- 最新獲得スコア: 小さなフォントで詳細表示

### 表示内容
```
Total Score: 1,250
─────────────
Chain: +420
Drop: +30
All Clear: +2100
─────────────
Last: +2550
```

### スコア更新タイミング
1. 連鎖完了時: スコア詳細と合計を即座に更新
2. ぷよ設置時: 落下ボーナス適用
3. 全消し時: 全消しボーナス適用

## 依存関係
- チケット014（連鎖システム統合）と密接に連携
- チケット006（Python連携）完了後に開始
- チケット007（Python UI実装）完了後に開始

## 優先度
高（ゲームの基本フィードバック）

## 推定工数
2-3時間（チケット014との並行作業）

## 完了条件
- ✅ ScoreCalculatorがPythonから利用できること
- ✅ 連鎖発生時にスコアが正確に計算・表示されること
- ✅ 落下ボーナスが適切に適用されること
- ✅ 全消し時にボーナスが適用されること
- ✅ UIでスコアの内訳が確認できること
- ✅ 累積スコアが正しく保存・表示されること

## 実装方針
1. **チケット014連携**: ChainSystemとの統合を最優先
2. **段階的表示**: 基本スコア→詳細→エフェクトの順で実装
3. **C++活用**: 既存のScoreCalculatorの正確な計算を活用