#include "pch.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "warp.hpp"

void warp(ENetEvent& event, const std::string_view text)
{
    if (text.length() <= sizeof("warp ") - 1) {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            "`^Usage: /warp <world name>"
        });
        return;
    }

    std::string world_name{ text.substr(sizeof("warp ") - 1) };

    packet::action(*event.peer, "log", std::format("msg| `6/warp {}``", world_name));
    packet::create(*event.peer, false, 0, { "OnSetFreezeState", 1 });
    packet::action(*event.peer, "log", std::format("msg|Magically warping to world `5{}``...", world_name));

    action::quit_to_exit(event, "", true);
    action::join_request(event, "", world_name);
}
