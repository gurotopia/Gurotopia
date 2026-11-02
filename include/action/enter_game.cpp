#include "pch.hpp"

#include "onVariant/RequestWorldSelectMenu.hpp"
#include "onVariant/RequestGazette.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetBux.hpp"
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
    if (active_holiday == holiday::HALLOWEEN)
    {
        on::ConsoleMessage(event.peer, "Growtopians have collectively released `20`` `9Corrupted Souls`` from the Tomb of Growganoth! Growganoth is pleased and unleashes the corrupted `2Tarantula Legs`` upon Growtopia!");
        send_varlist(event.peer, { "OnEventButtonDataSet", "HalloweenButton", 1, /*@todo*/ });
    }
    on::ConsoleMessage(event.peer, "`3Today is (null)!`` Your first (null) quest will give 1 `2Growtoken`` and all (null) quests will give 25% bonus points!");
    on::ConsoleMessage(event.peer, "`5Personal Settings active:`` `#Punch Joystick enabled``");
    
    send_inventory_state(event);

    on::SetBux(event);
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
    if (pPeer->game_version >= 5.47)
    {
        send_varlist(event.peer, {
            "OnSetFeatureEnableFlags",
            "EAcGAQoDEAQRBQ0JCAwCDw4="
        });
    }
}
