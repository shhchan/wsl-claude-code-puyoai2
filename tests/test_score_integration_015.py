#!/usr/bin/env python3
"""
スコアシステム統合テスト
ScoreCalculatorとChainSystemの連携テスト
"""

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__)))

import puyo_ai_platform as pap

def test_score_calculator_basic():
    """基本的なScoreCalculator機能テスト"""
    print("=== ScoreCalculator Basic Test ===")
    
    # ScoreCalculatorのインスタンス作成
    calc = pap.ScoreCalculator()
    
    # 落下ボーナステスト
    drop_bonus = calc.calculate_drop_bonus(5)
    print(f"Drop bonus for height 5: {drop_bonus}")
    expected = 5 + 1  # 落下距離 + 設置ボーナス
    assert drop_bonus == expected, f"Expected {expected}, got {drop_bonus}"
    
    # 空フィールドで全消し判定テスト
    field = pap.Field()
    is_all_clear = calc.is_all_clear(field)
    print(f"Empty field is all clear: {is_all_clear}")
    assert is_all_clear == True, "Empty field should be all clear"
    
    print("✓ Basic ScoreCalculator tests passed\n")

def test_chain_system_with_scores():
    """ChainSystemを使ったスコア計算テスト"""
    print("=== ChainSystem Score Integration Test ===")
    
    # フィールドを作成して簡単な連鎖パターンを設置
    field = pap.Field()
    
    # 簡単な4つ消しパターンを作成（1連鎖）
    # 赤ぷよを4つ設置
    field.set_puyo(pap.Position(2, 0), pap.PuyoColor.RED)
    field.set_puyo(pap.Position(3, 0), pap.PuyoColor.RED)
    field.set_puyo(pap.Position(2, 1), pap.PuyoColor.RED)
    field.set_puyo(pap.Position(3, 1), pap.PuyoColor.RED)
    
    # ChainSystemを作成
    chain_system = pap.ChainSystem(field)
    
    # 落下ボーナス付きで連鎖実行
    result = chain_system.execute_chains_with_drop_bonus(3)
    
    print(f"Chain result: {result.has_chains()}")
    print(f"Total chains: {result.total_chains}")
    print(f"Score details:")
    print(f"  Chain score: {result.score_result.chain_score}")
    print(f"  Drop score: {result.score_result.drop_score}")
    print(f"  All clear bonus: {result.score_result.all_clear_bonus}")
    print(f"  Total score: {result.score_result.total_score}")
    print(f"  Is all clear: {result.score_result.is_all_clear}")
    
    # 基本的な検証
    assert result.has_chains(), "Should have chains"
    assert result.total_chains >= 1, "Should have at least 1 chain"
    assert result.score_result.total_score > 0, "Should have positive score"
    assert result.score_result.drop_score == 4, "Drop bonus should be 3 + 1 = 4"
    
    print("✓ ChainSystem score integration tests passed\n")

def test_all_clear_bonus():
    """全消しボーナステスト"""
    print("=== All Clear Bonus Test ===")
    
    # フィールドに少量のぷよを設置して全消し状況を作る
    field = pap.Field()
    
    # 4つのぷよを設置（全部消えるように）
    field.set_puyo(pap.Position(0, 0), pap.PuyoColor.RED)
    field.set_puyo(pap.Position(1, 0), pap.PuyoColor.RED)
    field.set_puyo(pap.Position(0, 1), pap.PuyoColor.RED)
    field.set_puyo(pap.Position(1, 1), pap.PuyoColor.RED)
    
    chain_system = pap.ChainSystem(field)
    
    # 一度連鎖して全消しする
    result1 = chain_system.execute_chains()
    print(f"First chain - Is all clear: {result1.score_result.is_all_clear}")
    
    # 全消し後にもう一度別のぷよで連鎖（全消しボーナス適用）
    field.set_puyo(pap.Position(2, 0), pap.PuyoColor.BLUE)
    field.set_puyo(pap.Position(3, 0), pap.PuyoColor.BLUE)
    field.set_puyo(pap.Position(2, 1), pap.PuyoColor.BLUE)
    field.set_puyo(pap.Position(3, 1), pap.PuyoColor.BLUE)
    
    result2 = chain_system.execute_chains()
    print(f"Second chain - All clear bonus: {result2.score_result.all_clear_bonus}")
    
    print("✓ All clear bonus tests completed\n")

def main():
    """メインテスト実行"""
    print("Score System Integration Test")
    print("=" * 50)
    
    try:
        test_score_calculator_basic()
        test_chain_system_with_scores()
        test_all_clear_bonus()
        
        print("🎉 All tests passed successfully!")
        return 0
        
    except Exception as e:
        print(f"❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == "__main__":
    sys.exit(main())