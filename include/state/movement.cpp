#include "pch.hpp"
#include "movement.hpp"

void movement(ENetEvent& event, state state) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    
    pPeer->pos = state.pos;
    pPeer->facing_left = state.peer_state & 0x10;
    
    state.netid = pPeer->netid;
    state_visuals(*event.peer, std::move(state)); // finished.
}