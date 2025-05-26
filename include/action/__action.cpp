#include "pch.hpp"
#include "database/peer.hpp"

#include "logging_in.hpp"
#include "refresh_item_data.hpp"
#include "enter_game.hpp"

#include "dialog_return.hpp"
#include "friends.hpp"

#include "join_request.hpp"
#include "quit_to_exit.hpp"
#include "respawn.hpp"
#include "input.hpp"
#include "drop.hpp"
#include "wrench.hpp"

#include "quit.hpp"

#include "__action.hpp"

#include <functional>

std::unordered_map<std::string, std::function<void(ENetEvent, const std::string&)>> action_pool
{
    {"protocol", std::bind(&logging_in, std::placeholders::_1, std::placeholders::_2)},
    {"action|refresh_item_data", std::bind(&refresh_item_data, std::placeholders::_1, std::placeholders::_2)}, 
    {"action|enter_game", std::bind(&enter_game, std::placeholders::_1, std::placeholders::_2)},
    
    {"action|dialog_return", std::bind(&dialog_return, std::placeholders::_1, std::placeholders::_2)},
    {"action|friends", std::bind(&friends, std::placeholders::_1, std::placeholders::_2)},

    {"action|join_request", std::bind(&join_request, std::placeholders::_1, std::placeholders::_2, "")},
    {"action|quit_to_exit", std::bind(&quit_to_exit, std::placeholders::_1, std::placeholders::_2, false)},
    {"action|respawn", std::bind(&respawn, std::placeholders::_1, std::placeholders::_2)},
    {"action|respawn_spike", std::bind(&respawn, std::placeholders::_1, std::placeholders::_2)},
    {"action|input", std::bind(&input, std::placeholders::_1, std::placeholders::_2)},
    {"action|drop", std::bind(&drop, std::placeholders::_1, std::placeholders::_2)},
    {"action|wrench", std::bind(&wrench, std::placeholders::_1, std::placeholders::_2)},

    {"action|quit", std::bind(&quit, std::placeholders::_1, std::placeholders::_2)}
};