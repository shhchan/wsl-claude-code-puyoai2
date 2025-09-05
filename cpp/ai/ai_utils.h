#pragma once

#include "core/puyo_types.h"
#include "core/field.h"
#include <string>
#include <map>
#include <vector>
#include <fstream>

namespace puyo {
namespace ai {

// YAML設定読み込みユーティリティ
class ConfigLoader {
public:
    // YAML風設定ファイル読み込み（簡易実装）
    static std::map<std::string, std::string> load_config(const std::string& file_path) {
        std::map<std::string, std::string> config;
        std::ifstream file(file_path);
        
        if (!file.is_open()) {
            return config; // 空のconfigを返す
        }
        
        std::string line;
        std::string current_section = "";
        
        while (std::getline(file, line)) {
            line = trim(line);
            
            // コメント行やセクションヘッダーをスキップ
            if (line.empty() || line[0] == '#') continue;
            
            // セクション判定（インデントなしの行）
            if (line.find(':') != std::string::npos && line[0] != ' ') {
                size_t colon_pos = line.find(':');
                std::string key = trim(line.substr(0, colon_pos));
                std::string value = trim(line.substr(colon_pos + 1));
                
                if (!value.empty()) {
                    config[key] = value;
                } else {
                    current_section = key + ".";
                }
            }
            // サブキー（インデント有り）
            else if (line[0] == ' ' && line.find(':') != std::string::npos) {
                size_t colon_pos = line.find(':');
                std::string key = trim(line.substr(0, colon_pos));
                std::string value = trim(line.substr(colon_pos + 1));
                
                config[current_section + key] = value;
            }
        }
        
        return config;
    }
    
    // 数値変換ヘルパー
    static double get_double(const std::map<std::string, std::string>& config, 
                           const std::string& key, double default_value = 0.0) {
        auto it = config.find(key);
        if (it != config.end()) {
            try {
                return std::stod(it->second);
            } catch (...) {
                return default_value;
            }
        }
        return default_value;
    }
    
    static int get_int(const std::map<std::string, std::string>& config, 
                      const std::string& key, int default_value = 0) {
        auto it = config.find(key);
        if (it != config.end()) {
            try {
                return std::stoi(it->second);
            } catch (...) {
                return default_value;
            }
        }
        return default_value;
    }
    
    static bool get_bool(const std::map<std::string, std::string>& config, 
                        const std::string& key, bool default_value = false) {
        auto it = config.find(key);
        if (it != config.end()) {
            std::string value = to_lower(trim(it->second));
            return value == "true" || value == "yes" || value == "1";
        }
        return default_value;
    }
    
    static std::string get_string(const std::map<std::string, std::string>& config,
                                 const std::string& key, const std::string& default_value = "") {
        auto it = config.find(key);
        return (it != config.end()) ? it->second : default_value;
    }

private:
    static std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }
    
    static std::string to_lower(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            c = std::tolower(c);
        }
        return result;
    }
};

// フィールド分析ユーティリティ
class FieldAnalyzer {
public:
    // U字型評価（連鎖構築に重要）
    static double evaluate_u_shape(const Field& field) {
        std::vector<int> heights = get_column_heights(field);
        
        double u_score = 0.0;
        
        // U字の理想形：両端が高く、中央が低い
        // 左側の壁
        if (heights.size() >= 3) {
            if (heights[0] > heights[1] && heights[1] > heights[2]) {
                u_score += 10.0;
            }
        }
        
        // 右側の壁  
        if (heights.size() >= 3) {
            int n = heights.size();
            if (heights[n-1] > heights[n-2] && heights[n-2] > heights[n-3]) {
                u_score += 10.0;
            }
        }
        
        // 中央の谷
        if (heights.size() >= 6) {
            int center1 = heights[2];
            int center2 = heights[3];
            int avg_center = (center1 + center2) / 2;
            int avg_sides = (heights[0] + heights[1] + heights[4] + heights[5]) / 4;
            
            if (avg_sides > avg_center + 2) {
                u_score += 15.0; // 中央が低いほど良い
            }
        }
        
        return u_score;
    }
    
    // 連鎖発火可能性評価
    static int count_potential_chains(const Field& field) {
        int potential_chains = 0;
        
        // 各色について4個以上の塊を探す
        for (int color_int = static_cast<int>(PuyoColor::RED); 
             color_int <= static_cast<int>(PuyoColor::YELLOW); ++color_int) {
            
            PuyoColor color = static_cast<PuyoColor>(color_int);
            std::vector<std::vector<bool>> visited(FIELD_HEIGHT, std::vector<bool>(FIELD_WIDTH, false));
            
            for (int y = 0; y < FIELD_HEIGHT; ++y) {
                for (int x = 0; x < FIELD_WIDTH; ++x) {
                    if (!visited[y][x] && field.get_puyo(Position(x, y)) == color) {
                        int cluster_size = count_connected_puyos(field, x, y, color, visited);
                        if (cluster_size >= 4) {
                            potential_chains++;
                        }
                    }
                }
            }
        }
        
        return potential_chains;
    }
    
    // 色バランス評価
    static double evaluate_color_balance(const Field& field) {
        std::map<PuyoColor, int> color_counts;
        int total_puyos = 0;
        
        for (int y = 0; y < FIELD_HEIGHT; ++y) {
            for (int x = 0; x < FIELD_WIDTH; ++x) {
                PuyoColor color = field.get_puyo(Position(x, y));
                if (color != PuyoColor::EMPTY) {
                    color_counts[color]++;
                    total_puyos++;
                }
            }
        }
        
        if (total_puyos == 0) return 0.0;
        
        // 色の分散を計算（均等に近いほど良い）
        double ideal_ratio = 1.0 / 4.0; // 4色想定
        double balance_score = 0.0;
        
        for (const auto& pair : color_counts) {
            double actual_ratio = static_cast<double>(pair.second) / total_puyos;
            double diff = std::abs(actual_ratio - ideal_ratio);
            balance_score += std::max(0.0, 1.0 - diff * 2.0); // 差が小さいほど高スコア
        }
        
        return balance_score;
    }

private:
    static std::vector<int> get_column_heights(const Field& field) {
        std::vector<int> heights;
        
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            int height = 0;
            for (int y = FIELD_HEIGHT - 1; y >= 0; --y) {
                if (field.get_puyo(Position(x, y)) != PuyoColor::EMPTY) {
                    height = FIELD_HEIGHT - y;
                    break;
                }
            }
            heights.push_back(height);
        }
        
        return heights;
    }
    
    static int count_connected_puyos(const Field& field, int start_x, int start_y, 
                                   PuyoColor target_color, std::vector<std::vector<bool>>& visited) {
        if (start_x < 0 || start_x >= FIELD_WIDTH || 
            start_y < 0 || start_y >= FIELD_HEIGHT ||
            visited[start_y][start_x] ||
            field.get_puyo(Position(start_x, start_y)) != target_color) {
            return 0;
        }
        
        visited[start_y][start_x] = true;
        int count = 1;
        
        // 4方向に探索
        int dx[] = {0, 0, -1, 1};
        int dy[] = {-1, 1, 0, 0};
        
        for (int i = 0; i < 4; ++i) {
            int nx = start_x + dx[i];
            int ny = start_y + dy[i];
            count += count_connected_puyos(field, nx, ny, target_color, visited);
        }
        
        return count;
    }
};

} // namespace ai
} // namespace puyo