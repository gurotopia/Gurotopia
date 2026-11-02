#include "pch.hpp"

#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"

#include "warp.hpp"

void warp(ENetEvent& event, const std::string_view text)
{
    std::string world_name{ text.substr(strlen("warp ")) };
    for (char &c : world_name) c = std::toupper(c); // @note start -> START

    send_gameMessage(event.peer, {.message = std::format("action|log\nmsg| `6/warp {}``", world_name)});
    send_varlist(event.peer, { "OnSetFreezeState", 1 });
    send_gameMessage(event.peer, {.message = std::format("action|log\nmsg|Magically warping to world `5{}``...", world_name)});

    action::quit_to_exit(event, "", true);
    action::join_request(event, "", world_name);
}
