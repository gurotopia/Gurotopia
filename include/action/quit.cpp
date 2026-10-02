#include "pch.hpp"
#include "action/quit_to_exit.hpp"
#include "commands/friendsys.hpp"
#include "commands/trade.hpp"
#include "commands/legendary.hpp"
#include "quit.hpp"

void action::quit(ENetEvent& event, const std::string& header) 
{
    if (event.peer == nullptr) return;
    if (event.peer->data != nullptr) 
    {
        ::peer *pPeer = static_cast<::peer*>(event.peer->data);
        if (!pPeer->growid.empty()) friends_alert(*pPeer, false); // @note online friends see "logged off"
        trade_end_for(*pPeer, "left the game");                   // @note also drops trade requests
        action::quit_to_exit(event, "", true);
        legend_save(*pPeer); // @note Legendary Quest progress

        delete pPeer;
        event.peer->data = nullptr;
    }
    enet_peer_reset(event.peer);
}
