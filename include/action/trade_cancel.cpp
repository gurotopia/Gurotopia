#include "pch.hpp"
#include "tools/trade.hpp"
#include "trade_cancel.hpp"

void action::trade_cancel(ENetEvent& event, const std::string& header)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (ENetPeer *other = trade_partner(*pPeer); other != nullptr)
        trade_end(*other, std::format("{} `whas canceled the trade", pPeer->display_growid));

    pPeer->trade = {}; // @note our window is already closed by the client
}
