# チケット017: ぷよぷよエミュレータの微修正

## 概要

ぷよぷよエミュレータにおける２つの微修正を実装する：
1. 上への回転で壁キックが入るようにする修正
2. 組ぷよ設置可能判定の厳密なアルゴリズムへの修正

## 実装内容

### 1. 上への回転で壁キック実装（puyo_controller.cpp）

上（14段目）にぷよがある場合に壁キック機能を実装する。
- 対象ファイル: `cpp/core/puyo_controller.cpp`
- 実装内容: 上（14段目）にぷよがある場合、x方向に-1の移動を可能にする

### 2. 組ぷよ設置可能判定の修正（field.cpp）

ぷよぷよの厳密な判定チェックアルゴリズムを実装する。
- 対象ファイル: `cpp/core/field.cpp`
- 実装内容: `can_place_puyo_pair()` を以下のサンプルコードをもとに修正する

```cpp
bool Field::can_place(int x, int r) const {
        // ぷよの高さ情報を取得
        uint8_t heights[6] = {};
        for (int i = 0; i < 6; ++i) {
            heights[i] = 0;
            for (int y = get_height() - 1; y > 0; --y) {
                if (static_cast<int>(get_cell(i, y)) != 0) heights[i] = get_height() - y;
            }
        }
        // 14段目の情報（bit列）
        uint8_t row14 = 0;
        for (int i = 0; i < 6; ++i) {
            if (heights[i] == 14) row14 |= (1 << i);
        }
        // 回転方向のオフセット
        static const int dx[4] = {0, 1, 0, -1}; // UP, RIGHT, DOWN, LEFT
        static const int dy[4] = {-1, 0, 1, 0};
        // 0:UP, 1:RIGHT, 2:DOWN, 3:LEFT
        int dir = r;
        // 軸ぷよが14段目
        if (heights[x] + (dir == 2) > 12) return false;
        int child_x = x + dx[dir];
        if (child_x < 0 || child_x >= 6) return false;
        int child_y = heights[child_x] + (dir == 0);
        if (child_y == 13 && ((row14 >> child_x) & 1)) return false;
        // チェックリスト
        static const int check[6][4] = {
            {1, 0, -1, -1}, {1, -1, -1, -1}, {-1, -1, -1, -1}, {3, -1, -1, -1}, {3, 4, -1, -1}, {3, 4, 5, -1}
        };
        static const int check_12[6][6] = {
            {1, 2, 3, 4, 5, -1}, {2, 3, 4, 5, -1, -1}, {-1, -1, -1, -1, -1, -1}, {2, 1, 0, -1, -1, -1}, {3, 2, 1, 0, -1, -1}, {4, 3, 2, 1, 0, -1}
        };
        int check_x = x;
        if (dir == 1 && x >= 2) check_x += 1;
        else if (dir == 3 && x <= 2) check_x -= 1;
        int height_12_idx = -1;
        for (int i = 0; check[check_x][i] != -1; ++i) {
            if (heights[check[check_x][i]] > 12) return false;
            if (heights[check[check_x][i]] == 12 && height_12_idx == -1) height_12_idx = check[check_x][i];
        }
        if (height_12_idx == -1) return true;
        if (heights[1] > 11 && heights[3] > 11) return true;
        for (int i = 0; check_12[height_12_idx][i] != -1; ++i) {
            if (heights[check_12[height_12_idx][i]] > 11) break;
            if (heights[check_12[height_12_idx][i]] == 11) return true;
        }
        return false;
    }
```

## タスク一覧

- [ ] puyo_controller.cpp の壁キック実装
  - [ ] 現在のコードを理解・分析
  - [ ] 上への回転時の壁キック機能を実装
  - [ ] 14段目にぷよがある場合のx方向-1移動を実装
- [ ] field.cpp の設置可能判定修正
  - [ ] 現在の can_place_puyo_pair() を確認・分析
  - [ ] サンプルコードをベースに can_place() 関数を実装
  - [ ] 既存の can_place_puyo_pair() を新しいアルゴリズムに置き換え
- [ ] テストの実装
  - [ ] 壁キック機能のテストケース作成
  - [ ] 設置可能判定のテストケース作成
  - [ ] テスト実行とデバッグ
- [ ] ドキュメント作成
  - [ ] テスト結果記録ファイル作成（test-results-017.md）
  - [ ] 実装内容の詳細記録

## 検収基準

1. 上への回転時に14段目にぷよがある場合、壁キックが適切に動作すること
2. 組ぷよの設置可能判定が厳密なアルゴリズムに基づいて正しく動作すること
3. 既存の動作を破綻させないこと
4. 全てのテストケースが合格すること

## 備考

- 実装前に現在のコードの動作を十分理解すること
- テスト駆動開発を心がけ、変更が既存機能に悪影響を与えないことを確認すること
- パフォーマンスへの影響を最小限に抑えること