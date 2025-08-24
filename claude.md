# ぷよぷよAI開発基盤プロジェクト仕様書

## プロジェクト概要

### 目的
ぷよぷよeスポーツSteam版の「ぷよぷよ通ルール」に準拠したAI開発基盤を構築する。エミュレータ方式（時間経過による自動落下なし）で実装し、様々なAIアルゴリズムを簡単に追加・比較できるプラットフォームを提供する。

### システム構成
- **C++**: ゲームエンジン、AIモデル（高速計算が必要な部分）
- **Python**: UI、ゲーム制御、設定管理、結果分析
- **連携**: pybind11による高効率なC++↔Python連携

### 対応モード
- **対戦モード**: AI vs AI、AI vs Human
- **とことんモード**: 一人プレイ（延々と連鎖を組み続ける練習・スコアアタック）

## ぷよぷよ通ルール仕様

詳細は `puyo_game_information.md` を参照してください。

## 実装要件

### C++コア部分（高速化対象）
- **ゲームエンジン**: ルール処理、フィールド操作
- **連鎖シミュレーション**: 高速計算（目標: 10,000回/秒以上）
- **AIモデル**: 探索型，強化学習型などを想定

### Python制御部分（柔軟性重視）
- **UI・描画**: pygame等による可視化
- **ゲーム制御**: 対戦管理、トーナメント制御
- **AI管理**: プラグイン型AI登録・作成システム
- **分析・ログ**: 結果分析、性能評価、可視化

### 連携仕様
- **pybind11**: C++↔Python高効率連携
- **データ変換**: GameState、Action等の相互変換
- **動的ライブラリ**: C++AIの動的ロード対応

## パフォーマンス要件

### ベンチマーク目標
- **連鎖シミュレーション**: 10,000回/秒以上
- **AI思考時間**: 1手あたり300ms以内
- **配置可能性判定**: 1ms以内
- **ゲーム状態コピー**: 1ms以内

### 最適化戦略
- フィールド上の計算をビット演算で実施
- 動的メモリ確保を削減

## 使用例

### 基本的な対戦実行
```python
import puyo_ai_platform as pap

# システム初期化
platform = pap.Platform()

# AI作成
ai1 = platform.create_ai('minimax', {'search_depth': 5})
ai2 = platform.create_ai('mcts', {'simulation_count': 1000})

# 対戦実行
result = platform.battle(ai1, ai2, rounds=100, render_realtime=True)

# 結果分析
platform.analyze_results(result)
```

### トーナメント実行
```python
# 複数AI準備
ais = [
    platform.create_ai('random'),
    platform.create_ai('minimax_depth3'),
    platform.create_ai('mcts_1000'),
    platform.create_ai('neural_network_v1'),
]

# トーナメント実行
tournament_result = platform.tournament(ais, rounds_per_match=50)
platform.generate_tournament_report(tournament_result)
```

## 拡張性

### AI追加の容易さ
- 統一インターフェースによる簡単なAI追加
- C++/Python両対応
- プラグイン型設計

### 将来の拡張
- 他ルール対応（フィーバーモード等）
- ネットワーク対戦
- 大規模分散計算
- Web UI対応

---

## TODO管理について

チケット内のTODOアイテムは以下の記法で管理します：

- `- [ ]`: 未完了のタスク
- `- [x]`: 完了したタスク

例：
```markdown
- [ ] 実装予定のタスク
- [x] 完了済みのタスク
```

実際に実施した内容について，チケットにまとめて記録します．

## テスト記録の義務化

**重要**: 機能実装完了後は**必ず**以下の手順でテスト結果を記録してください：

1. **テスト実装後**：機能のテストを実装・実行
2. **テスト結果記録**：`docs/test-results-XXX.md`ファイルを作成
3. **記録内容**：
   - テスト対象の詳細
   - テスト環境情報
   - 全テストケースの実行手順と期待結果
   - 実際の実行結果とコマンド
   - 成功率と総合評価
   - 実装検収基準の確認

Claude Codeは機能実装完了時に自動的にテスト結果記録を作成する必要があります。

## テストの管理について

機能実装に対して，必ずテストを作成してください．
プロジェクトのテストは以下のように管理します：

### テストファイル管理
- テスト用ディレクトリ: `tests/`
- テストファイルは削除せずに`tests/`ディレクトリで保管
- 過去のテストファイルやバインディングファイルは削除しない
- 再現可能なテスト手順を文書化

### テスト結果記録
- 各チケットのテスト結果は`docs/test-results-XXX.md`に記録
- テスト実施方法、結果、環境情報を含める
- テストの再現手順を明記

### 実行環境（重要）
- Pythonコマンドの実行時は必ず`venv`の仮想環境を利用する
- `source venv/bin/activate`で仮想環境をアクティベートしてから実行

## ビルドとテストの実行方法

### ビルド方法
```bash
# 推奨ビルド方法（venv環境使用）
source venv/bin/activate && python setup.py build_ext --inplace
```

### テスト実行方法例
```bash
# ChainSystem基本テスト例
source venv/bin/activate && python -c "
import puyo_ai_platform as pap
gm = pap.GameManager(pap.GameMode.TOKOTON)
gm.add_player('Test Player', pap.PlayerType.HUMAN)
player = gm.get_player(0)
chain_system = player.get_chain_system()
field = player.get_field()

# 4つ隣接する赤ぷよを配置
for i in range(4):
    field.set_puyo(pap.Position(i, 0), pap.PuyoColor.RED)

chain_result = chain_system.execute_chains()
print(f'has_chains={chain_result.has_chains()}, total_chains={chain_result.total_chains}')
print(f'score={chain_result.score_result.total_score}')
"

# UI統合テスト
source venv/bin/activate && cd python/ui && python game_controller.py
```

---

*この仕様書は、ぷよぷよAI開発基盤の実装指針として、厳密なゲームルール再現と高性能な計算基盤の両立を目指します。C++による高速計算とPythonによる柔軟な制御の組み合わせにより、研究・実用の両方に対応できる強力なプラットフォームを提供します。