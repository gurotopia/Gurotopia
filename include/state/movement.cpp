#include "pch.hpp"
#include "movement.hpp"

void movement(ENetEvent& event, state state) 
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    
    peer->pos = state.pos;
    peer->facing_left = state.peer_state & 0x10;
    
    state.netid = peer->netid;
    state_visuals(*event.peer, std::move(state)); // finished.
}