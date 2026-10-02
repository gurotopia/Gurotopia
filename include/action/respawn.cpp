#include "pch.hpp"
#include "commands/buffs.hpp"
#include "respawn.hpp"

void action::respawn(ENetEvent& event, const std::string& header) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->god_mode || (header.find("respawn_spike") != std::string::npos && buff_active(*pPeer, buff::SPIKE)))
    {
        // client froze itself on "death" - release it without moving the player
        send_varlist(event.peer, { "OnSetFreezeState" }, pPeer->netid);
        return;
    }

    send_varlist(event.peer, { "OnSetFreezeState", 2 }, pPeer->netid);
    send_varlist(event.peer, { "OnKilled"}, pPeer->netid);

    // @note wait 1900 milliseconds···

    send_varlist(event.peer, {
        "OnSetPos", 
        CL_Vec2f{pPeer->rest_pos.x, pPeer->rest_pos.y}
    }, pPeer->netid, 1900);
    send_varlist(event.peer, { "OnSetFreezeState"}, pPeer->netid, 1900);
}