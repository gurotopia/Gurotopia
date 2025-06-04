#include "pch.hpp"
#include "database/peer.hpp"
#include "network/packet.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "enter_game.hpp"

#include "tools/string_view.hpp"

void enter_game(ENetEvent event, const std::string& header) 
{
    auto& peer = _peer[event.peer];
    peer->user_id = fnv1a(peer->ltoken[0]); // @note FNV-1A is to proeprly downgrade std::hash to integer (Growtopia Standards)
    if (peer->role == role::moderator) peer->prefix = "8@";
    else if (peer->role == role::developer) peer->prefix = "6@";

    OnRequestWorldSelectMenu(event);
    gt_packet(*event.peer, false, 0, {
        "OnConsoleMessage", 
        std::format("Welcome back, `{}{}````. No friends are online.", 
            peer->prefix, peer->ltoken[0]).c_str()
    }); 
    gt_packet(*event.peer, false, 0, {"OnConsoleMessage", "`5Personal Settings active:`` `#Can customize profile``"}); 
    gt_packet(*event.peer, false, 0, {
        "UpdateMainMenuTheme", 
        0,
        4226000383u,
        4226000383u,
        "4226000383|4226000383|80543231|80543231|1554912511|1554912511"
    }); 
    gt_packet(*event.peer, false, 0, {
        "OnSetBux",
        peer->gems,
        1,
        1
    });
    gt_packet(*event.peer, false, 0, {"SetHasGrowID", 1, peer->ltoken[0], ""}); 

    {
        std::time_t now = std::time(nullptr);
        std::tm time = *std::localtime(&now);

        gt_packet(*event.peer, false, 0, {
            "OnTodaysDate",
            time.tm_mon + 1,
            time.tm_mday,
            0u, // @todo
            0u // @todo
        }); 
    } // @note delete now, time
}