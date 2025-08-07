#pragma once

#include "field.h"
#include "chain_system.h"
#include <vector>
#include <queue>

namespace puyo {

// おじゃまぷよ管理
struct GarbagePuyo {
    int count;          // 個数
    int source_player;  // 送信元プレイヤー（将来の拡張用）
    
    GarbagePuyo(int count = 0, int source_player = -1) 
        : count(count), source_player(source_player) {}
};

// おじゃまぷよシステム結果
struct GarbageResult {
    int sent_garbage;       // 送信したおじゃまぷよ数
    int received_garbage;   // 受信したおじゃまぷよ数
    int offset_garbage;     // 相殺されたおじゃまぷよ数
    int placed_garbage;     // 実際に配置されたおじゃまぷよ数
    
    GarbageResult() : sent_garbage(0), received_garbage(0), 
                     offset_garbage(0), placed_garbage(0) {}
};

class GarbageSystem {
private:
    Field* field_;
    std::queue<GarbagePuyo> pending_garbage_;  // 予告おじゃまぷよ
    int total_pending_;                        // 予告おじゃまぷよ総数
    int accumulated_score_;                    // 蓄積されたスコア（70点未満の端数）
    
    static constexpr int GARBAGE_RATE = 70;    // 70点につき1個
    
public:
    explicit GarbageSystem(Field* field);
    
    // おじゃまぷよ送信計算（蓄積スコアを考慮）
    int calculate_garbage_to_send(int score);
    
    // 蓄積スコアの取得・設定
    int get_accumulated_score() const { return accumulated_score_; }
    void set_accumulated_score(int score) { accumulated_score_ = score; }
    
    // 予告おじゃまぷよの追加
    void add_pending_garbage(int count, int source_player = -1);
    
    // 相殺処理
    int offset_garbage_with_score(int score);
    
    // おじゃまぷよの降下・配置
    GarbageResult drop_pending_garbage();
    
    // 予告おじゃまぷよ情報の取得
    int get_pending_garbage_count() const { return total_pending_; }
    bool has_pending_garbage() const { return total_pending_ > 0; }
    
    // クリア
    void clear_pending_garbage();
    
    // おじゃまぷよのフィールド配置
    bool place_garbage_on_field(int count);
    
    // デバッグ用
    std::string get_garbage_info() const;
    
private:
    // おじゃまぷよの配置パターン計算（N段+r個方式）
    std::vector<Position> calculate_garbage_positions(int count);
    
    // N段+r個での配置計算
    void calculate_layers_and_remainder(int count, int& full_layers, int& remainder_count);
    
    // ランダムな列選択（余り個数分）
    std::vector<int> select_random_columns(int count);
    
    // フィールド上部からの配置
    void place_garbage_from_top(const std::vector<Position>& positions);
};

} // namespace puyo