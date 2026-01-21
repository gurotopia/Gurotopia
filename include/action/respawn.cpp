#include "pch.hpp"
#include "respawn.hpp"

void action::respawn(ENetEvent& event, const std::string& header) 
{
    packet::create(*event.peer, true, 0, { 
        "OnSetFreezeState", 
        2 
    });
    packet::create(*event.peer, true, 0,{ "OnKilled" });
    // @note wait 1900 milliseconds···
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    packet::create(*event.peer, true, 1900, {
        "OnSetPos", 
        std::vector<float>{peer->rest_pos.front(), peer->rest_pos.back()}
    });
    packet::create(*event.peer, true, 1900, { "OnSetFreezeState" });
}