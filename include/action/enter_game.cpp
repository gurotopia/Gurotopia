#include "pch.hpp"
#include "database/peer.hpp"
#include "network/packet.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "enter_game.hpp"

#include "tools/string_view.hpp"

#include <format>

void enter_game(ENetEvent event, const std::string& header) 
{
    std::call_once(_peer[event.peer]->entered_game, [&]() 
    {
        _peer[event.peer]->user_id = fnv1a(_peer[event.peer]->ltoken[0]); // @note FNV-1A is to proeprly downgrade std::hash to integer (Growtopia Standards)

        OnRequestWorldSelectMenu(event);

        gt_packet(*event.peer, false, 0, {"OnConsoleMessage", std::format("Welcome back, `w`w{}````. No friends are online.", _peer[event.peer]->ltoken[0]).c_str()}); 
        gt_packet(*event.peer, false, 0, {
            "UpdateMainMenuTheme", 
            0,
            4226000383u,
            4226000383u,
            "4226000383|4226000383|80543231|80543231|1554912511|1554912511"
        }); 
        gt_packet(*event.peer, false, 0, {
           "OnSetBux",
            _peer[event.peer]->gems,
            1,
            1
        });
        gt_packet(*event.peer, false, 0, {"SetHasGrowID", 1, _peer[event.peer]->ltoken[0], ""}); 
    });
}