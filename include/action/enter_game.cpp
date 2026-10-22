#include "pch.hpp"
#include "onVariant/RequestWorldSelectMenu.hpp"
#include "onVariant/RequestGazette.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetBux.hpp"
#include "tools/create_dialog.hpp"
#include "automate/holiday.hpp"

#include "enter_game.hpp"

void action::enter_game(ENetEvent& event, const std::string& header) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    pPeer->display_growid = std::format("`w{}``", pPeer->growid);
    on::ConsoleMessage(event.peer, 
        std::format("Welcome back, {}. No friends are online.", 
            pPeer->display_growid
        )
    );
    on::ConsoleMessage(event.peer, holiday_greeting().second);
    on::ConsoleMessage(event.peer, "`5Personal Settings active:`` `#Punch Joystick enabled``");
    
    send_inventory_state(event);

    on::SetBux(event);
    if (pPeer->level.front() > 1)
    {
        send_varlist(event.peer, { "OnPlayerLeveledUp", pPeer->level.front() });
    }
    send_varlist(event.peer, { "OnSetVouchers", 0 });
    send_varlist(event.peer, { "SetHasGrowID", 1u, pPeer->growid.c_str(), "" });
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

    send_varlist(event.peer, { "OnRequestHoustonPricing" });

    on::RequestWorldSelectMenu(event);
    on::RequestGazette(event);

    send_data(*event.peer, compress_state(::gamePacket{ .type = 0x16 /*PACKET_PING_REQUEST*/ }));
    /* for v5.47+ client */
    send_varlist(event.peer, {
        "OnSetFeatureEnableFlags",
        "EA4NBQgJAQoPAgQRBwYDEAw=" // @todo Dw0JEQQMEAMPAgYBDgUICg==
    });
}
