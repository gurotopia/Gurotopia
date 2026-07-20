#include "pch.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "on/ConsoleMessage.hpp"

#include "warp.hpp"

void warp(ENetEvent& event, const std::string_view text)
{
    std::string world_name{ text.substr(strlen("warp ")) };
    std::for_each(world_name.begin(), world_name.end(), [](char& c) { c = std::toupper(c); });

    send_action(*event.peer, "log", std::format("msg| `6/warp {}``", world_name));
    send_varlist(event.peer, { "OnSetFreezeState", 1 });
    send_action(*event.peer, "log", std::format("msg|Magically warping to world `5{}``...", world_name));

    action::quit_to_exit(event, "", true);
    action::join_request(event, "", world_name);
}
