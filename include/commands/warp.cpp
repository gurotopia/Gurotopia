#include "pch.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include "action/respawn.hpp"
#include "warp.hpp"

void warp(ENetEvent& event, const std::string_view text)
{
    if (::peer *pCursed = static_cast<::peer*>(event.peer->data); pCursed->curse_until > std::time(nullptr) && pCursed->recent_worlds.back() == "HELL")
    {
        action::respawn(event, "");
        return;
    }
    std::string world_name{ text.substr(strlen("warp ")) };
    for (char &c : world_name) c = std::toupper(c); // @note start -> START

    send_action(*event.peer, "log", std::format("msg| `6/warp {}``", world_name));
    send_varlist(event.peer, { "OnSetFreezeState", 1 });
    send_action(*event.peer, "log", std::format("msg|Magically warping to world `5{}``...", world_name));

    action::quit_to_exit(event, "", true);
    action::join_request(event, "", world_name);
}
