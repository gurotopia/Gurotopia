#include "pch.hpp"

#include "logging_in.hpp"
#include "refresh_item_data.hpp"
#include "enter_game.hpp"

#include "dialog_return.hpp"
#include "friends.hpp"

#include "join_request.hpp"
#include "quit_to_exit.hpp"
#include "respawn.hpp"
#include "setSkin.hpp"
#include "input.hpp"
#include "drop.hpp"
#include "info.hpp"
#include "trash.hpp"
#include "wrench.hpp"
#include "itemfavourite.hpp"
#include "inventoryfavuitrigger.hpp"
#include "store.hpp"
#include "buy.hpp"

#include "quit.hpp"

#include "__action.hpp"

std::unordered_map<std::string, std::function<void(ENetEvent&, const std::string&)>> action_pool
{
    {"protocol", std::bind(&action::logging_in, std::placeholders::_1, std::placeholders::_2)},
    {"action|refresh_item_data", std::bind(&action::refresh_item_data, std::placeholders::_1, std::placeholders::_2)}, 
    {"action|enter_game", std::bind(&action::enter_game, std::placeholders::_1, std::placeholders::_2)},
    
    {"action|dialog_return", std::bind(&action::dialog_return, std::placeholders::_1, std::placeholders::_2)},
    {"action|friends", std::bind(&action::friends, std::placeholders::_1, std::placeholders::_2)},

    {"action|join_request", std::bind(&action::join_request, std::placeholders::_1, std::placeholders::_2, "")},
    {"action|quit_to_exit", std::bind(&action::quit_to_exit, std::placeholders::_1, std::placeholders::_2, false)},
    {"action|respawn", std::bind(&action::respawn, std::placeholders::_1, std::placeholders::_2)},
    {"action|respawn_spike", std::bind(&action::respawn, std::placeholders::_1, std::placeholders::_2)},
    {"action|setSkin", std::bind(&action::setSkin, std::placeholders::_1, std::placeholders::_2)},
    {"action|input", std::bind(&action::input, std::placeholders::_1, std::placeholders::_2)},
    {"action|drop", std::bind(&action::drop, std::placeholders::_1, std::placeholders::_2)},
    {"action|info", std::bind(&action::info, std::placeholders::_1, std::placeholders::_2)},
    {"action|trash", std::bind(&action::trash, std::placeholders::_1, std::placeholders::_2)},
    {"action|wrench", std::bind(&action::wrench, std::placeholders::_1, std::placeholders::_2)},
    {"action|itemfavourite", std::bind(&action::itemfavourite, std::placeholders::_1, std::placeholders::_2)},
    {"action|inventoryfavuitrigger", std::bind(&action::inventoryfavuitrigger, std::placeholders::_1, std::placeholders::_2)},
    {"action|store", std::bind(&action::store, std::placeholders::_1, std::placeholders::_2)},
    {"action|buy", std::bind(&action::buy, std::placeholders::_1, std::placeholders::_2)},

    {"action|quit", std::bind(&action::quit, std::placeholders::_1, std::placeholders::_2)}
};