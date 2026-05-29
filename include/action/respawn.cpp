#include "pch.hpp"
#include "respawn.hpp"

void action::respawn(ENetEvent& event, const std::string& header) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    packet::create(*event.peer, true, 0, { 
        "OnSetFreezeState", 
        2 
    });
    packet::create(*event.peer, true, 0,{ "OnKilled" });
    // @note wait 1900 milliseconds···
    packet::create(*event.peer, true, 1900, {
        "OnSetPos", 
        std::vector<float>{pPeer->rest_pos.x, pPeer->rest_pos.y}
    });
    packet::create(*event.peer, true, 1900, { "OnSetFreezeState" });
}