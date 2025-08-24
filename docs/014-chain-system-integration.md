# チケット014: 連鎖システム統合実装

## 概要
C++のChainSystemクラスをPython UIで活用し、連鎖実行と連鎖数表示機能を実装

## 現状分析

### 実装されているもの
- ✅ C++のChainSystemクラス（連鎖検出・実行・スコア計算）
- ✅ PlayerクラスにChainSystemのアクセサ
- ⚠️ Python bindingsではChainSystem関連が未露出
- ⚠️ ぷよ設置後に連鎖処理が自動実行されていない

### 実装されていないもの
- ❌ ChainSystem関連のPython露出
- ❌ ぷよ設置後の自動連鎖実行
- ❌ UIでの連鎖数表示
- ❌ 連鎖アニメーション（基本表示のみでOK）

## 詳細タスク

### 1. ChainSystemのPython連携
- [x] `python_bindings.cpp`にChainSystemResult構造体追加
- [x] `python_bindings.cpp`にChainSystemクラス追加
- [x] `execute_chains`, `execute_chains_with_drop_bonus`メソッド露出
- [x] PlayerクラスからのChainSystemアクセサ追加

### 2. 連鎖実行ロジック統合
- [x] `GameController._place_current_pair()`に連鎖処理追加
- [x] ぷよ設置→重力→連鎖実行の正しい順序実装
- [x] 連鎖結果の取得と統計更新

### 3. UI表示機能実装
- [x] `renderer.py`に連鎖数表示エリア追加
- [x] 最新連鎖数の表示（例: "3 Chain!"）
- [x] 連鎖発生時の視覚的フィードバック（色変更など）

### 4. 統合テスト
- [x] 4つ以上のぷよ隣接時の自動連鎖実行確認
- [x] 多段連鎖の正しい実行確認
- [x] 連鎖数の正確な表示確認
- [x] 既存UI機能の動作確認

## 技術詳細

### C++側既存機能
```cpp
struct ChainSystemResult {
    std::vector<ChainResult> chain_results;  // 各連鎖の詳細
    ScoreResult score_result;                // スコア結果
    int total_chains;                        // 総連鎖数
    bool has_chains() const;
};

class ChainSystem {
    ChainSystemResult execute_chains();                           // 連鎖実行
    ChainSystemResult execute_chains_with_drop_bonus(int height); // 落下ボーナス付き
};

class Player {
    ChainSystem& get_chain_system();  // ChainSystemアクセス
};
```

### Python統合目標
```python
# 連鎖実行
player = game_manager.get_player(0)
chain_system = player.get_chain_system()

# ぷよ設置後の連鎖処理
field.apply_gravity()
chain_result = chain_system.execute_chains()

if chain_result.has_chains():
    print(f"Chain: {chain_result.total_chains}")
    # 統計更新とUI表示
```

## UI実装方針

### 連鎖数表示位置
- 右側UIエリアの上部（プレイヤー情報の下）
- 大きなフォントで「X Chain!」形式
- 連鎖なしの場合は非表示または「Ready」表示

### 表示タイミング
1. 連鎖実行中: 「Chaining...」
2. 連鎖完了: 「X Chain!」（2秒間表示）
3. 通常状態: 非表示

## 依存関係
- チケット006（Python連携）完了後に開始
- チケット007（Python UI実装）完了後に開始

## 優先度
高（ゲームの基本メカニクス）

## 推定工数
3-4時間

## 完了条件
- ✅ ChainSystemがPythonから利用できること
- ✅ 4つ以上隣接したぷよで自動的に連鎖が実行されること
- ✅ 連鎖数が画面上に正しく表示されること
- ✅ 多段連鎖（2連鎖、3連鎖など）が正常動作すること
- ✅ 連鎖処理による重力・消去が正しく動作すること
- ✅ 既存のゲーム機能が正常動作すること

## 実装方針
1. **段階的統合**: Python連携→基本実行→UI表示の順で実装
2. **C++活用**: 既存のChainSystemの機能を最大限活用
3. **視覚重視**: 連鎖の発生を分かりやすくユーザーに伝える