#include "player.h"
#include <sstream>

namespace puyo {

Player::Player(int player_id, const std::string& name, PlayerType type)
    : player_id_(player_id), name_(name), type_(type), state_(PlayerState::ACTIVE),
      controller_(&field_), chain_system_(&field_), garbage_system_(&field_) {
    initialize_game();
}

void Player::initialize_game() {
    field_.clear();
    next_generator_.initialize_next_sequence();
    garbage_system_.clear_pending_garbage();
    chain_system_.get_score_calculator().reset();
    stats_ = PlayerStats();
    state_ = PlayerState::ACTIVE;
    
    // 最初のぷよペアを設定
    controller_.set_current_pair(next_generator_.get_current_pair());
}

void Player::reset_game() {
    initialize_game();
}

bool Player::is_game_over() const {
    return state_ == PlayerState::DEFEATED || field_.is_game_over();
}

void Player::update_stats(const ChainSystemResult& chain_result, const GarbageResult& garbage_result) {
    if (chain_result.has_chains()) {
        stats_.total_score += chain_result.score_result.total_score;
        stats_.total_chains += chain_result.total_chains;
        stats_.max_chain = std::max(stats_.max_chain, chain_result.total_chains);
    }
    
    stats_.sent_garbage += garbage_result.sent_garbage;
    stats_.received_garbage += garbage_result.received_garbage;
}

std::string Player::get_status() const {
    std::ostringstream oss;
    
    oss << "Player " << player_id_ << " (" << name_ << ")\n";
    oss << "Type: " << (type_ == PlayerType::HUMAN ? "Human" : "AI") << "\n";
    oss << "State: ";
    
    switch (state_) {
        case PlayerState::ACTIVE:   oss << "Active"; break;
        case PlayerState::DEFEATED: oss << "Defeated"; break;
        case PlayerState::PAUSED:   oss << "Paused"; break;
    }
    oss << "\n";
    
    oss << "Score: " << stats_.total_score << "\n";
    oss << "Chains: " << stats_.total_chains << " (Max: " << stats_.max_chain << ")\n";
    oss << "Garbage: Sent " << stats_.sent_garbage << ", Received " << stats_.received_garbage << "\n";
    oss << "Pending Garbage: " << garbage_system_.get_pending_garbage_count() << "\n";
    
    if (is_game_over()) {
        oss << "GAME OVER\n";
    }
    
    return oss.str();
}

} // namespace puyo