#include "pch.hpp"
#include "onVariant/RequestWorldSelectMenu.hpp"
#include "onVariant/RequestGazette.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetBux.hpp"
#include "tools/create_dialog.hpp"
#include "automate/holiday.hpp"

#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "commands/friendsys.hpp"
#include "commands/legendary.hpp"
#include "enter_game.hpp"

void action::enter_game(ENetEvent& event, const std::string& header) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->nickname.empty()) pPeer->display_growid = (pPeer->role >= DEVELOPER) ? std::format("`6@{}``", pPeer->growid) : (pPeer->role >= MODERATOR) ? std::format("`5@{}``", pPeer->growid) : std::format("`w{}``", pPeer->growid);
    on::ConsoleMessage(event.peer, 
        std::format("Welcome back, {}. {}", pPeer->display_growid, friends_welcome(*pPeer))
    );
    friends_alert(*pPeer, true); // @note tells your online friends
    legend_load(*pPeer);
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

    send_data(*event.peer, compress_state(::gamePacket{ .type = 0x16 /*PACKET_PING_REQUEST*/ }));
    /* for v5.47+ client */
    send_varlist(event.peer, {
        "OnSetFeatureEnableFlags",
        "EA8DEAcGAgEOBQgKCQ0MEQQ=" // @todo Dw0JEQQMEAMPAgYBDgUICg==
    });

    /* back into the same world after /god reloaded items.dat - done 2 seconds later by the timer */
    if (!pPeer->god_rejoin.empty())
    {
        if (pPeer->netid != 0) action::quit_to_exit(event, "", true); // @note the game left the world when it reloaded
        pPeer->god_rejoin_at = std::time(nullptr) + 2;
    }
}
