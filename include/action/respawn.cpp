#include "pch.hpp"
#include "respawn.hpp"

void action::respawn(ENetEvent& event, const std::string& header) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    send_varlist(event.peer, { "OnSetFreezeState", 2 }, pPeer->netid);
    send_varlist(event.peer, { "OnKilled"}, pPeer->netid);

    // @note wait 1900 milliseconds···

    send_varlist(event.peer, {
        "OnSetPos", 
        CL_Vec2f{pPeer->rest_pos.x, pPeer->rest_pos.y}
    }, pPeer->netid, 1900);
    send_varlist(event.peer, { "OnSetFreezeState"}, pPeer->netid, 1900);
}