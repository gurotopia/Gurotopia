#include "pch.hpp"
#include "movement.hpp"

void movement(ENetEvent& event, state state) 
{
    auto &peer = _peer[event.peer];
    
    peer->pos = { state.pos[0] / 32.0f, state.pos[1] / 32.0f };
    peer->facing_left = state.peer_state & 0x10;
    
    state_visuals(event, std::move(state)); // finished.
}