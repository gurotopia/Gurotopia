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

    // daily bonus: give 500 gems once per calendar day
    {
        std::time_t now = std::time(nullptr);
        std::tm today = *std::localtime(&now);
        std::tm last_daily_tm{};
        if (pPeer->last_daily > 0)
            last_daily_tm = *std::localtime(&pPeer->last_daily);

        bool same_day = (pPeer->last_daily > 0) &&
            (today.tm_year == last_daily_tm.tm_year) &&
            (today.tm_mon  == last_daily_tm.tm_mon) &&
            (today.tm_mday == last_daily_tm.tm_mday);

        if (!same_day)
        {
            pPeer->gems += 500;
            pPeer->last_daily = now;
            on::ConsoleMessage(event.peer, "`5Daily Bonus: +500 Gems!``");
        }
    }

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
    /* for v5.47+ client */
    send_varlist(event.peer, {
        "OnSetFeatureEnableFlags",
        "EA8DEAcGAgEOBQgKCQ0MEQQ=" // @todo Dw0JEQQMEAMPAgYBDgUICg==
    });
}