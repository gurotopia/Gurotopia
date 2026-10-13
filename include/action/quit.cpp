#include "pch.hpp"
#include "action/quit_to_exit.hpp"
#include "action/trade.hpp"
#include "quit.hpp"

void action::quit(ENetEvent& event, const std::string& header) 
{
    if (event.peer == nullptr) return;
    if (event.peer->data != nullptr)
    {
        action::quit_to_exit(event, "", true);
        trade_end_for(*event.peer);
        delete static_cast<::peer*>(event.peer->data);
        event.peer->data = nullptr;
    }
    enet_peer_reset(event.peer);
}
