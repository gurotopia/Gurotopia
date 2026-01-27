#include "pch.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "tools/string.hpp"
#include "on/SetBux.hpp"
#include "enter_game.hpp"

void action::enter_game(ENetEvent& event, const std::string& header) 
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    if (peer->slots.empty()) // @note if peer has no items: assume they are a new player.
    {
        peer->emplace({18, 1}); // @note Fist
        peer->emplace({32, 1}); // @note Wrench
        peer->emplace({9640, 1}); // @note My First World Lock
    }

    on::RequestWorldSelectMenu(event);

    peer->prefix = (peer->role == MODERATOR) ? "#@" : (peer->role == DEVELOPER) ? "8@" : peer->prefix;
    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage", 
        std::format("Welcome back, `{}{}````. No friends are online.", 
            peer->prefix, peer->ltoken[0]).c_str()
    }); 
    packet::create(*event.peer, false, 0, {"OnConsoleMessage", "`5Personal Settings active:`` `#Can customize profile``"});
    
    send_inventory_state(event);
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

    send_data(*event.peer, compress_state(::state{
        .type = 0x16 // @noote PACKET_PING_REQUEST
    }));
}