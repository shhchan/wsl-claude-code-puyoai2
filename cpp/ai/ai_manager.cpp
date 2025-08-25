#include "ai_manager.h"

namespace puyo {
namespace ai {

// グローバルAIマネージャーの静的メンバー定義
std::unique_ptr<AIManager> GlobalAIManager::instance_ = nullptr;

} // namespace ai
} // namespace puyo