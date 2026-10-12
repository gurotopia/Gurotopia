#include "pch.hpp"
#include "set_icon_state.hpp"

/* @note the "..." bubble while typing */
void set_icon_state(ENetEvent& event, ::gamePacket gamePacket)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->netid == 0) return; // @note not in a world

    gamePacket.netid = pPeer->netid;
    state_visuals(*event.peer, std::move(gamePacket));
}
