#include "database/items.hpp"
#include "database/peer.hpp"
#include "database/world.hpp"
#include "movement.hpp"

void movement(ENetEvent event, state state) 
{
    auto &peer = _peer[event.peer];
    
    peer->pos[0] = state.pos[0] / 32.0f;
    peer->pos[1] = state.pos[1] / 32.0f;
    peer->facing_left = state.peer_state & 16;
    state_visuals(event, std::move(state)); // finished.
}