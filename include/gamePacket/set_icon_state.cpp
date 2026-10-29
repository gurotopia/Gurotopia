#include "pch.hpp"

#include "set_icon_state.hpp"

void set_icon_state(ENetEvent& event, ::gamePacket gamePacket)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    if (pPeer->netid == 0) return;
    const float value = gamePacket.punch.x;
    if (value != 0.0f && value != 1.0f && value != 2.0f) return;
    const u_char icon = static_cast<u_char>(value);
    if (icon == pPeer->icon) return;
    pPeer->icon = icon;

    const ::blob data = compress_state({.type = 0x12, .netid = pPeer->netid, .punch = ::pos{static_cast<int>(icon), 0}});
    peers(pPeer->recent_worlds.back(), PeerSearch::WORLD, [&](ENetPeer *peer)
    {
        if (peer != event.peer) send_data(*peer, data);
    });
}
