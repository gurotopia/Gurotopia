#include "pch.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "tools/string.hpp"
#include "on/SetBux.hpp"
#include "enter_game.hpp"

void action::enter_game(ENetEvent& event, const std::string& header) 
{
    auto &peer = _peer[event.peer];

    peer->user_id = fnv1a(peer->ltoken[0]); // @note this is to proeprly downgrade std::hash to integer size hash (Growtopia Standards)
    peer->prefix = (peer->role == MODERATOR) ? "#@" : (peer->role == DEVELOPER) ? "8@" : peer->prefix;

    on::RequestWorldSelectMenu(event);
    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage", 
        std::format("Welcome back, `{}{}````. No friends are online.", 
            peer->prefix, peer->ltoken[0]).c_str()
    }); 
    packet::create(*event.peer, false, 0, {"OnConsoleMessage", "`5Personal Settings active:`` `#Can customize profile``"});
    packet::create(*event.peer, false, 0, {
        "UpdateMainMenuTheme", 
        0,
        4226000383u,
        4226000383u,
        "4226000383|4226000383|80543231|80543231|1554912511|1554912511"
    });
    on::SetBux(event);
    packet::create(*event.peer, false, 0, {"SetHasGrowID", 1, peer->ltoken[0].c_str(), ""}); 

    {
        std::time_t now = std::time(nullptr);
        std::tm time = *std::localtime(&now);

        packet::create(*event.peer, false, 0, {
            "OnTodaysDate",
            time.tm_mon + 1,
            time.tm_mday,
            0u, // @todo
            0u // @todo
        }); 
    } // @note delete now, time
}