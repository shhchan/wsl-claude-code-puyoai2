cmake_minimum_required(VERSION 3.15)
project(puyo_ai_platform)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# コンパイラ固有のオプション
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra")
endif()

# pybind11の検索と設定
find_package(pybind11 REQUIRED)

# ソースファイルの指定
file(GLOB_RECURSE CPP_CORE_SOURCES "cpp/core/*.cpp")
file(GLOB_RECURSE CPP_AI_SOURCES "cpp/ai/*.cpp")
file(GLOB_RECURSE CPP_BINDINGS_SOURCES "cpp/bindings/*.cpp")

# インクルードディレクトリの指定
include_directories(cpp)

# C++ライブラリの作成
add_library(puyo_core STATIC ${CPP_CORE_SOURCES})
set_property(TARGET puyo_core PROPERTY POSITION_INDEPENDENT_CODE ON)

# AI ライブラリは AI ソースファイルがある場合のみ作成
if(CPP_AI_SOURCES)
    add_library(puyo_ai STATIC ${CPP_AI_SOURCES})
    set_property(TARGET puyo_ai PROPERTY POSITION_INDEPENDENT_CODE ON)
    set(AI_LIB puyo_ai)
else()
    message(STATUS "No AI sources found, skipping AI library creation")
    set(AI_LIB "")
endif()

# Python拡張モジュールの作成
pybind11_add_module(puyo_ai_platform ${CPP_BINDINGS_SOURCES})
target_link_libraries(puyo_ai_platform PRIVATE puyo_core ${AI_LIB})

# コンパイル時の定義
target_compile_definitions(puyo_ai_platform PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})