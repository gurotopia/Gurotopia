#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/SetClothing.hpp"
#include "database/world.hpp"
#include "undress.hpp"

/* /undress -> takes off everything you're wearing */
void undress(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    for (float &slot : pPeer->clothing) slot = 0.0f;

    pPeer->update_effects();
    on::SetClothing(*event.peer);
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [pPeer](ENetPeer &p) { ::peer *o = static_cast<::peer*>(p.data); if (o && o->user_id != pPeer->user_id) on::SetClothing(p, *pPeer); });

    on::ConsoleMessage(event.peer, "`2All clothing removed.``");
}
