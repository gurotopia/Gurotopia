#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "tools/trade.hpp"
#include "trade_started.hpp"

void action::trade_started(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->trade.netid == 0) return;

    if (trade_partner(*pPeer) != nullptr) // @note both asked each other, the trade is live
    {
        trade_update(*event.peer, false);
        return;
    }
    ENetPeer *target = trade_target(*pPeer);
    if (target == nullptr)
    {
        trade_end(*event.peer, "`4That player left the world.``");
        return;
    }
    on::ConsoleMessage(target, std::format(
        "`#TRADE ALERT:`` {} `owants to trade with you!  To start, use the `wWrench`` on that person's wrench icon, or type `w/trade {}``",
        pPeer->display_growid, pPeer->growid
    ));
    send_action(*target, "play_sfx", "file|audio/cash_register.wav\ndelayMS|0");
}
