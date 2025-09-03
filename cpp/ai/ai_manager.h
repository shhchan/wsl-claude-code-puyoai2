#pragma once

#include "ai_base.h"
#include "random_ai.h"
#include "chain_search_ai.h"
#include "rl_player_ai.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

namespace puyo {
namespace ai {

// AI登録情報
struct AIInfo {
    std::string name;
    std::string type;
    std::string version;
    std::string description;
    AIFactory factory;
    
    AIInfo(const std::string& n, const std::string& t, const std::string& v, 
           const std::string& d, const AIFactory& f)
        : name(n), type(t), version(v), description(d), factory(f) {}
};

class AIManager {
private:
    std::unordered_map<std::string, AIInfo> registered_ais_;
    
public:
    AIManager() {
        // 組み込みAIを登録
        register_builtin_ais();
    }
    
    // AI登録
    bool register_ai(const std::string& name, 
                    const std::string& type,
                    const std::string& version,
                    const std::string& description,
                    const AIFactory& factory) {
        
        if (registered_ais_.find(name) != registered_ais_.end()) {
            return false; // 既に登録済み
        }
        
        registered_ais_.emplace(name, AIInfo(name, type, version, description, factory));
        return true;
    }
    
    // AI作成
    std::unique_ptr<AIBase> create_ai(const std::string& name, 
                                     const AIParameters& params = {}) {
        auto it = registered_ais_.find(name);
        if (it == registered_ais_.end()) {
            return nullptr;
        }
        
        return it->second.factory(params);
    }
    
    // 登録済みAI一覧取得
    std::vector<std::string> get_registered_ai_names() const {
        std::vector<std::string> names;
        for (const auto& pair : registered_ais_) {
            names.push_back(pair.first);
        }
        return names;
    }
    
    // AI情報取得
    bool get_ai_info(const std::string& name, AIInfo& info) const {
        auto it = registered_ais_.find(name);
        if (it == registered_ais_.end()) {
            return false;
        }
        info = it->second;
        return true;
    }
    
    // AI登録数
    size_t get_ai_count() const {
        return registered_ais_.size();
    }
    
    // AI登録解除
    bool unregister_ai(const std::string& name) {
        return registered_ais_.erase(name) > 0;
    }
    
    // すべてクリア
    void clear_all() {
        registered_ais_.clear();
    }
    
    // デバッグ用
    std::string get_status() const {
        std::string status = "AIManager Status:\n";
        status += "Registered AIs: " + std::to_string(registered_ais_.size()) + "\n";
        
        for (const auto& pair : registered_ais_) {
            const AIInfo& info = pair.second;
            status += "  - " + info.name + " (" + info.type + " v" + info.version + "): " + info.description + "\n";
        }
        
        return status;
    }
    
private:
    void register_builtin_ais() {
        // RandomAI登録
        register_ai(
            "random",
            "Random",
            "1.0",
            "Random action AI for testing",
            [](const AIParameters& params) -> std::unique_ptr<AIBase> {
                return std::make_unique<RandomAI>(params);
            }
        );
        
        // ChainSearchAI登録
        register_ai(
            "chain_search",
            "ChainSearch",
            "1.0",
            "Chain search AI using minimax algorithm",
            [](const AIParameters& params) -> std::unique_ptr<AIBase> {
                return std::make_unique<ChainSearchAI>(params);
            }
        );
        
        // RLPlayerAI登録
        register_ai(
            "rl_player",
            "RLPlayer",
            "1.0",
            "Reinforcement Learning AI using Q-Learning",
            [](const AIParameters& params) -> std::unique_ptr<AIBase> {
                return std::make_unique<RLPlayerAI>(params);
            }
        );
        
        // 他の組み込みAIもここに追加可能
    }
};

// グローバルAIマネージャー（シングルトン）
class GlobalAIManager {
private:
    static std::unique_ptr<AIManager> instance_;
    
public:
    static AIManager* get_instance() {
        if (!instance_) {
            instance_ = std::make_unique<AIManager>();
        }
        return instance_.get();
    }
    
    static void reset_instance() {
        instance_.reset();
    }
};

} // namespace ai
} // namespace puyo