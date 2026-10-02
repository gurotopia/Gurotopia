#include "pch.hpp"
#include <set>
#include "onVariant/Spawn.hpp"
#include "onVariant/SetClothing.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "onVariant/CountryState.hpp"
#include "database/world.hpp"
#include "invis.hpp"

extern std::string country_state_for(const ::peer &p); // @note commands/legendary.cpp: flag + title flags

static std::set<int> ghost_before{}; // @note players who had /ghost on before /invis


void invis(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can use this.``");
        return;
    }

    if (pPeer->netid == 0)
    {
        on::ConsoleMessage(event.peer, "`4You must be in a world.``");
        return;
    }

    pPeer->state ^= S_INVISIBLE;
    const bool hidden = (pPeer->state & S_INVISIBLE) != 0;
    const int uid = static_cast<int>(pPeer->user_id);
    if (hidden)
    {
        if (pPeer->state & S_GHOST) ghost_before.insert(uid); else ghost_before.erase(uid);
        pPeer->state |= S_GHOST; // @note /invis turns /ghost on
    }
    else
    {
        if (!ghost_before.contains(uid)) pPeer->state &= ~S_GHOST; // @note /ghost stays on if you had it before /invis
        ghost_before.erase(uid);
    }

    const std::string netid = std::format("netID|{}\n", pPeer->netid);
    const std::string pId   = std::format("pId|{}\n", pPeer->user_id);

    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD,
        [pPeer, hidden, netid, pId](ENetPeer& peer)
    {
        ::peer *pOthers = static_cast<::peer*>(peer.data);
        if (!pOthers) return;

        if (pOthers->user_id == pPeer->user_id) // @note your own game: eyes, mouth, name and flag only while hidden
        {
            on::SetClothing(peer);
            return;
        }

        if (hidden)
        {
            send_varlist(&peer, { "OnRemove", netid, pId });
        }
        else
        {
            on::Spawn(peer, pPeer->netid, pPeer->user_id, pPeer->pos,
                      pPeer->display_growid, country_state_for(*pPeer),
                      pPeer->role, pPeer->role >= DEVELOPER, false);
            on::SetClothing(peer, *pPeer);
            send_varlist(&peer, { "OnNameChanged", pPeer->display_growid }, pPeer->netid); // @note title look
            on::CountryStateOf(peer, *pPeer);
            send_varlist(&peer, { "OnSetPos", CL_Vec2f{ pPeer->pos.x, pPeer->pos.y } }, pPeer->netid);
        }
    });

    on::ConsoleMessage(event.peer,
        hidden ? "`2You are now invisible to other players.``"
               : "`2You are now visible again.``");
}