#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>

#include "core/field.h"
#include "core/game_manager.h"
#include "core/player.h"
#include "core/puyo_types.h"
#include "core/puyo_controller.h"
#include "core/next_generator.h"
#include "core/chain_system.h"
#include "core/chain_detector.h"
#include "core/score_calculator.h"

namespace py = pybind11;

PYBIND11_MODULE(puyo_ai_platform, m) {
    m.doc() = "Puyo Puyo AI Development Platform";
    
    // カスタム例外
    py::register_exception<std::invalid_argument>(m, "InvalidArgumentError");
    py::register_exception<std::runtime_error>(m, "RuntimeError");
    py::register_exception<std::out_of_range>(m, "OutOfRangeError");
    
    // PuyoColor列挙型
    py::enum_<puyo::PuyoColor>(m, "PuyoColor")
        .value("EMPTY", puyo::PuyoColor::EMPTY)
        .value("RED", puyo::PuyoColor::RED)
        .value("GREEN", puyo::PuyoColor::GREEN)
        .value("BLUE", puyo::PuyoColor::BLUE)
        .value("YELLOW", puyo::PuyoColor::YELLOW)
        .value("PURPLE", puyo::PuyoColor::PURPLE)
        .value("GARBAGE", puyo::PuyoColor::GARBAGE);
    
    // Rotation列挙型
    py::enum_<puyo::Rotation>(m, "Rotation")
        .value("UP", puyo::Rotation::UP)
        .value("RIGHT", puyo::Rotation::RIGHT)
        .value("DOWN", puyo::Rotation::DOWN)
        .value("LEFT", puyo::Rotation::LEFT);
    
    // Position構造体
    py::class_<puyo::Position>(m, "Position")
        .def(py::init<int, int>(), py::arg("x") = 0, py::arg("y") = 0)
        .def_readwrite("x", &puyo::Position::x)
        .def_readwrite("y", &puyo::Position::y)
        .def("is_valid", &puyo::Position::is_valid)
        .def("to_bit_index", &puyo::Position::to_bit_index)
        .def("__eq__", &puyo::Position::operator==)
        .def("__repr__", [](const puyo::Position& p) {
            return "Position(x=" + std::to_string(p.x) + ", y=" + std::to_string(p.y) + ")";
        });
    
    // PuyoPair構造体
    py::class_<puyo::PuyoPair>(m, "PuyoPair")
        .def(py::init<puyo::PuyoColor, puyo::PuyoColor, puyo::Position, puyo::Rotation>(),
             py::arg("axis") = puyo::PuyoColor::EMPTY,
             py::arg("child") = puyo::PuyoColor::EMPTY,
             py::arg("pos") = puyo::Position(2, 11),
             py::arg("rot") = puyo::Rotation::UP)
        .def_readwrite("axis", &puyo::PuyoPair::axis)
        .def_readwrite("child", &puyo::PuyoPair::child)
        .def_readwrite("pos", &puyo::PuyoPair::pos)
        .def_readwrite("rot", &puyo::PuyoPair::rot)
        .def("get_child_position", &puyo::PuyoPair::get_child_position);
    
    // Field クラス
    py::class_<puyo::Field>(m, "Field")
        .def(py::init<>())
        .def("clear", &puyo::Field::clear)
        .def("get_puyo", [](const puyo::Field& self, const puyo::Position& pos) {
            if (!pos.is_valid()) {
                throw std::out_of_range("Position is out of range");
            }
            return self.get_puyo(pos);
        })
        .def("set_puyo", [](puyo::Field& self, const puyo::Position& pos, puyo::PuyoColor color) {
            if (!pos.is_valid()) {
                throw std::out_of_range("Position is out of range");
            }
            self.set_puyo(pos, color);
        })
        .def("remove_puyo", [](puyo::Field& self, const puyo::Position& pos) {
            if (!pos.is_valid()) {
                throw std::out_of_range("Position is out of range");
            }
            self.remove_puyo(pos);
        })
        .def("can_place_at_row14", [](const puyo::Field& self, int column) {
            if (column < 0 || column >= puyo::FIELD_WIDTH) {
                throw std::out_of_range("Column is out of range");
            }
            return self.can_place_at_row14(column);
        })
        .def("mark_row14_used", [](puyo::Field& self, int column) {
            if (column < 0 || column >= puyo::FIELD_WIDTH) {
                throw std::out_of_range("Column is out of range");
            }
            self.mark_row14_used(column);
        })
        .def("is_row14_used", [](const puyo::Field& self, int column) {
            if (column < 0 || column >= puyo::FIELD_WIDTH) {
                throw std::out_of_range("Column is out of range");
            }
            return self.is_row14_used(column);
        })
        .def("can_place_puyo_pair", &puyo::Field::can_place_puyo_pair)
        .def("place_puyo_pair", &puyo::Field::place_puyo_pair)
        .def("apply_gravity", &puyo::Field::apply_gravity)
        .def("is_game_over", &puyo::Field::is_game_over)
        .def("to_string", &puyo::Field::to_string)
        .def("__copy__", [](const puyo::Field& self) {
            return puyo::Field(self);
        })
        .def("__deepcopy__", [](const puyo::Field& self, py::dict) {
            return puyo::Field(self);
        });
    
    // GameMode列挙型
    py::enum_<puyo::GameMode>(m, "GameMode")
        .value("TOKOTON", puyo::GameMode::TOKOTON)
        .value("VERSUS", puyo::GameMode::VERSUS);
    
    // GameState列挙型
    py::enum_<puyo::GameState>(m, "GameState")
        .value("WAITING", puyo::GameState::WAITING)
        .value("PLAYING", puyo::GameState::PLAYING)
        .value("PAUSED", puyo::GameState::PAUSED)
        .value("FINISHED", puyo::GameState::FINISHED);
    
    // GameStep列挙型
    py::enum_<puyo::GameStep>(m, "GameStep")
        .value("PUYO_SPAWN", puyo::GameStep::PUYO_SPAWN)
        .value("PLAYER_INPUT", puyo::GameStep::PLAYER_INPUT)
        .value("PUYO_PLACE", puyo::GameStep::PUYO_PLACE)
        .value("GRAVITY", puyo::GameStep::GRAVITY)
        .value("CHAIN_PROCESS", puyo::GameStep::CHAIN_PROCESS)
        .value("GARBAGE_DROP", puyo::GameStep::GARBAGE_DROP)
        .value("WIN_CHECK", puyo::GameStep::WIN_CHECK)
        .value("NEXT_TURN", puyo::GameStep::NEXT_TURN);
    
    // PlayerType列挙型
    py::enum_<puyo::PlayerType>(m, "PlayerType")
        .value("HUMAN", puyo::PlayerType::HUMAN)
        .value("AI", puyo::PlayerType::AI);
    
    // MoveCommand列挙型
    py::enum_<puyo::MoveCommand>(m, "MoveCommand")
        .value("LEFT", puyo::MoveCommand::LEFT)
        .value("RIGHT", puyo::MoveCommand::RIGHT)
        .value("ROTATE_CW", puyo::MoveCommand::ROTATE_CW)
        .value("ROTATE_CCW", puyo::MoveCommand::ROTATE_CCW)
        .value("DROP", puyo::MoveCommand::DROP)
        .value("NONE", puyo::MoveCommand::NONE);
    
    // PlayerState列挙型
    py::enum_<puyo::PlayerState>(m, "PlayerState")
        .value("ACTIVE", puyo::PlayerState::ACTIVE)
        .value("DEFEATED", puyo::PlayerState::DEFEATED)
        .value("PAUSED", puyo::PlayerState::PAUSED);
    
    // PlayerStats構造体
    py::class_<puyo::PlayerStats>(m, "PlayerStats")
        .def(py::init<>())
        .def_readwrite("total_score", &puyo::PlayerStats::total_score)
        .def_readwrite("total_chains", &puyo::PlayerStats::total_chains)
        .def_readwrite("max_chain", &puyo::PlayerStats::max_chain)
        .def_readwrite("sent_garbage", &puyo::PlayerStats::sent_garbage)
        .def_readwrite("received_garbage", &puyo::PlayerStats::received_garbage);
    
    // GameResult構造体
    py::class_<puyo::GameResult>(m, "GameResult")
        .def(py::init<>())
        .def_readwrite("mode", &puyo::GameResult::mode)
        .def_readwrite("winner_id", &puyo::GameResult::winner_id)
        .def_readwrite("player_stats", &puyo::GameResult::player_stats)
        .def_readwrite("total_turns", &puyo::GameResult::total_turns);
    
    // PuyoController クラス
    py::class_<puyo::PuyoController>(m, "PuyoController")
        .def(py::init<puyo::Field*>())
        .def("set_current_pair", &puyo::PuyoController::set_current_pair)
        .def("get_current_pair", &puyo::PuyoController::get_current_pair)
        .def("execute_command", &puyo::PuyoController::execute_command)
        .def("move_left", &puyo::PuyoController::move_left)
        .def("move_right", &puyo::PuyoController::move_right)
        .def("move_down", &puyo::PuyoController::move_down)
        .def("rotate_clockwise", &puyo::PuyoController::rotate_clockwise)
        .def("rotate_counter_clockwise", &puyo::PuyoController::rotate_counter_clockwise)
        .def("place_current_pair", &puyo::PuyoController::place_current_pair)
        .def("can_move_left", &puyo::PuyoController::can_move_left)
        .def("can_move_right", &puyo::PuyoController::can_move_right)
        .def("can_move_down", &puyo::PuyoController::can_move_down)
        .def("can_rotate_clockwise", &puyo::PuyoController::can_rotate_clockwise)
        .def("can_rotate_counter_clockwise", &puyo::PuyoController::can_rotate_counter_clockwise);
    
    // NextGenerator クラス
    py::class_<puyo::NextGenerator>(m, "NextGenerator")
        .def(py::init<>())
        .def(py::init<unsigned int>())
        .def("set_active_colors", &puyo::NextGenerator::set_active_colors)
        .def("get_active_colors", &puyo::NextGenerator::get_active_colors, py::return_value_policy::reference)
        .def("initialize_next_sequence", &puyo::NextGenerator::initialize_next_sequence)
        .def("get_current_pair", &puyo::NextGenerator::get_current_pair)
        .def("get_next_pair", &puyo::NextGenerator::get_next_pair)
        .def("advance_to_next", &puyo::NextGenerator::advance_to_next)
        .def("to_string", &puyo::NextGenerator::to_string);
    
    // ChainGroup構造体
    py::class_<puyo::ChainGroup>(m, "ChainGroup")
        .def(py::init<>())
        .def_readwrite("color", &puyo::ChainGroup::color)
        .def_readwrite("positions", &puyo::ChainGroup::positions)
        .def("size", &puyo::ChainGroup::size);
    
    // ChainResult構造体
    py::class_<puyo::ChainResult>(m, "ChainResult")
        .def(py::init<>())
        .def_readwrite("groups", &puyo::ChainResult::groups)
        .def_readwrite("chain_level", &puyo::ChainResult::chain_level)
        .def_readwrite("total_cleared", &puyo::ChainResult::total_cleared)
        .def_readwrite("color_count", &puyo::ChainResult::color_count)
        .def("has_chains", &puyo::ChainResult::has_chains)
        .def("clear", &puyo::ChainResult::clear);
    
    // ScoreResult構造体
    py::class_<puyo::ScoreResult>(m, "ScoreResult")
        .def(py::init<>())
        .def_readwrite("chain_score", &puyo::ScoreResult::chain_score)
        .def_readwrite("drop_score", &puyo::ScoreResult::drop_score)
        .def_readwrite("all_clear_bonus", &puyo::ScoreResult::all_clear_bonus)
        .def_readwrite("total_score", &puyo::ScoreResult::total_score)
        .def_readwrite("is_all_clear", &puyo::ScoreResult::is_all_clear);
    
    // ScoreCalculator クラス
    py::class_<puyo::ScoreCalculator>(m, "ScoreCalculator")
        .def(py::init<>())
        .def("calculate_chain_score", &puyo::ScoreCalculator::calculate_chain_score)
        .def("calculate_drop_bonus", &puyo::ScoreCalculator::calculate_drop_bonus)
        .def("is_all_clear", &puyo::ScoreCalculator::is_all_clear)
        .def("set_pending_all_clear_bonus", &puyo::ScoreCalculator::set_pending_all_clear_bonus)
        .def("get_pending_all_clear_bonus", &puyo::ScoreCalculator::get_pending_all_clear_bonus)
        .def("reset", &puyo::ScoreCalculator::reset);
    
    // ChainSystemResult構造体
    py::class_<puyo::ChainSystemResult>(m, "ChainSystemResult")
        .def(py::init<>())
        .def_readwrite("chain_results", &puyo::ChainSystemResult::chain_results)
        .def_readwrite("score_result", &puyo::ChainSystemResult::score_result)
        .def_readwrite("total_chains", &puyo::ChainSystemResult::total_chains)
        .def("has_chains", &puyo::ChainSystemResult::has_chains);
    
    // ChainSystem クラス
    py::class_<puyo::ChainSystem>(m, "ChainSystem")
        .def(py::init<puyo::Field*>())
        .def("execute_chains", &puyo::ChainSystem::execute_chains)
        .def("execute_chains_with_drop_bonus", &puyo::ChainSystem::execute_chains_with_drop_bonus)
        .def("would_cause_chain", &puyo::ChainSystem::would_cause_chain)
        .def("count_potential_chains", &puyo::ChainSystem::count_potential_chains)
        .def("get_chain_info", &puyo::ChainSystem::get_chain_info)
        .def("get_score_calculator", (puyo::ScoreCalculator& (puyo::ChainSystem::*)()) &puyo::ChainSystem::get_score_calculator, py::return_value_policy::reference);

    // Player クラス
    py::class_<puyo::Player>(m, "Player")
        .def("get_id", &puyo::Player::get_id)
        .def("get_name", &puyo::Player::get_name, py::return_value_policy::reference)
        .def("get_type", &puyo::Player::get_type)
        .def("get_state", &puyo::Player::get_state)
        .def("set_state", &puyo::Player::set_state)
        .def("get_field", (puyo::Field& (puyo::Player::*)()) &puyo::Player::get_field, py::return_value_policy::reference)
        .def("get_next_generator", (puyo::NextGenerator& (puyo::Player::*)()) &puyo::Player::get_next_generator, py::return_value_policy::reference)
        .def("get_chain_system", (puyo::ChainSystem& (puyo::Player::*)()) &puyo::Player::get_chain_system, py::return_value_policy::reference)
        .def("get_stats", &puyo::Player::get_stats, py::return_value_policy::reference)
        .def("initialize_game", &puyo::Player::initialize_game)
        .def("reset_game", &puyo::Player::reset_game)
        .def("is_game_over", &puyo::Player::is_game_over)
        .def("get_status", &puyo::Player::get_status);
    
    // GameManager クラス
    py::class_<puyo::GameManager>(m, "GameManager")
        .def(py::init<puyo::GameMode>())
        .def("add_player", &puyo::GameManager::add_player)
        .def("get_player", (puyo::Player* (puyo::GameManager::*)(int)) &puyo::GameManager::get_player, py::return_value_policy::reference_internal)
        .def("start_game", &puyo::GameManager::start_game)
        .def("pause_game", &puyo::GameManager::pause_game)
        .def("resume_game", &puyo::GameManager::resume_game)
        .def("reset_game", &puyo::GameManager::reset_game)
        .def("finish_game", &puyo::GameManager::finish_game)
        .def("execute_step", &puyo::GameManager::execute_step)
        .def("execute_full_turn", &puyo::GameManager::execute_full_turn)
        .def("set_input_callback", &puyo::GameManager::set_input_callback)
        .def("get_mode", &puyo::GameManager::get_mode)
        .def("get_state", &puyo::GameManager::get_state)
        .def("get_current_step", &puyo::GameManager::get_current_step)
        .def("get_current_player", &puyo::GameManager::get_current_player)
        .def("get_turn_count", &puyo::GameManager::get_turn_count)
        .def("enable_time_limit", &puyo::GameManager::enable_time_limit)
        .def("disable_time_limit", &puyo::GameManager::disable_time_limit)
        .def("is_game_finished", &puyo::GameManager::is_game_finished)
        .def("get_winner", &puyo::GameManager::get_winner)
        .def("get_game_status", &puyo::GameManager::get_game_status);
    
    // 定数をエクスポート
    m.attr("FIELD_WIDTH") = puyo::FIELD_WIDTH;
    m.attr("FIELD_HEIGHT") = puyo::FIELD_HEIGHT;
    m.attr("VISIBLE_HEIGHT") = puyo::VISIBLE_HEIGHT;
    m.attr("HIDDEN_HEIGHT") = puyo::HIDDEN_HEIGHT;
    m.attr("COLOR_COUNT") = puyo::COLOR_COUNT;
}