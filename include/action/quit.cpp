#include "pch.hpp"
#include "action/quit_to_exit.hpp"
#include "quit.hpp"

void action::quit(ENetEvent& event, const std::string& header) 
{
    action::quit_to_exit(event, "", true);
    
    if (event.peer == nullptr) return;
    if (event.peer->data != nullptr) 
    {
        event.peer->data = nullptr;
        _peer.erase(event.peer);
    }
    enet_peer_reset(event.peer);
}