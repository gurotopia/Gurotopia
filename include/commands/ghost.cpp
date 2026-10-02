#include "pch.hpp"
#include "onVariant/SetClothing.hpp"
#include "database/world.hpp"
#include "ghost.hpp"

void ghost(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    pPeer->state ^= S_GHOST;

    on::SetClothing(*event.peer);

    if ((pPeer->state & S_INVISIBLE) == S_INVISIBLE) return; // @note nobody can see you anyway

    // let everyone else in the world see the change too
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [pPeer](ENetPeer &p)
    {
        ::peer *pOthers = static_cast<::peer*>(p.data);
        if (!pOthers || pOthers->user_id == pPeer->user_id) return;
        on::SetClothing(p, *pPeer);
    });
}