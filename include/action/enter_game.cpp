#include "pch.hpp"
#include "on/RequestWorldSelectMenu.hpp"
#include "on/RequestGazette.hpp"
#include "on/ConsoleMessage.hpp"
#include "on/SetBux.hpp"
#include "tools/create_dialog.hpp"
#include "automate/holiday.hpp"

#include "enter_game.hpp"

void action::enter_game(ENetEvent& event, const std::string& header) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->slots.empty()) // @note if peer has no items: assume they are a new player.
    {
        pPeer->emplace({18, 1}); // @note Fist
        pPeer->emplace({32, 1}); // @note Wrench
        pPeer->emplace({9640, 1}); // @note My First World Lock
    }
    pPeer->prefix = (pPeer->role == MODERATOR) ? "#@" : (pPeer->role == DEVELOPER) ? "8@" : pPeer->prefix;
    on::ConsoleMessage(event.peer, 
        std::format("Welcome back, `{}{}````. No friends are online.", 
            pPeer->prefix, pPeer->growid
        )
    );
    on::ConsoleMessage(event.peer, holiday_greeting().second);
    on::ConsoleMessage(event.peer, "`5Personal Settings active:`` `#Can customize profile``");
    
    send_inventory_state(event);
    on::SetBux(event);
    send_varlist(event.peer, { "SetHasGrowID", 1, pPeer->growid.c_str(), "" });
    {
        std::tm time = localtime();

        send_varlist(event.peer, {
            "OnTodaysDate",
            time.tm_mon + 1,
            time.tm_mday,
            0u, // @todo
            0u // @todo
        });
    } // @note delete time

    on::RequestWorldSelectMenu(event);
    on::RequestGazette(event);
    send_data(*event.peer, compress_state(::state{
        .type = 0x16 // @noote PACKET_PING_REQUEST
    }));
}
