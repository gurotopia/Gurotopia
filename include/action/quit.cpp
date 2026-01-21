#include "pch.hpp"
#include "action/quit_to_exit.hpp"
#include "quit.hpp"

void action::quit(ENetEvent& event, const std::string& header) 
{
    action::quit_to_exit(event, "", true);
    
    if (event.peer == nullptr) return;
    if (event.peer->data != nullptr) 
    {
        printf("delete peer\n");
        delete static_cast<::peer*>(event.peer->data);
        event.peer->data = nullptr;
    }
    enet_peer_reset(event.peer);
}