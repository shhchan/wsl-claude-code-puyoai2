#!/usr/bin/env python3
"""
基本環境テスト実行スクリプト
"""

def test_python_environment():
    """Python環境の動作確認"""
    import sys
    print(f"✅ Python version: {sys.version}")
    
    try:
        import numpy
        print(f"✅ NumPy version: {numpy.__version__}")
    except ImportError:
        print("❌ NumPy not found")
        return False
    
    try:
        import pygame
        print(f"✅ Pygame version: {pygame.version.ver}")
    except ImportError:
        print("❌ Pygame not found")
        return False
    
    try:
        import pybind11
        print(f"✅ pybind11 version: {pybind11.__version__}")
    except ImportError:
        print("❌ pybind11 not found")
        return False
    
    return True

def test_pybind11_integration():
    """pybind11連携テスト"""
    try:
        import test_module
        result = test_module.add(1, 2)
        expected = 3
        if result == expected:
            print(f"✅ pybind11 integration test: {result} (expected: {expected})")
            return True
        else:
            print(f"❌ pybind11 integration test failed: {result} (expected: {expected})")
            return False
    except ImportError:
        print("❌ test_module not found. Please build it first.")
        return False

if __name__ == "__main__":
    print("=== 基本環境テスト開始 ===")
    
    python_ok = test_python_environment()
    pybind_ok = test_pybind11_integration()
    
    print("\n=== テスト結果 ===")
    if python_ok and pybind_ok:
        print("✅ すべてのテストが成功しました")
    else:
        print("❌ 一部のテストが失敗しました")