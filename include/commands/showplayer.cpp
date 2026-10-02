#include "pch.hpp"
#include "onVariant/Spawn.hpp"
#include "onVariant/SetClothing.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "showplayer.hpp"

/* /showplayer -> reappear from /invis with a particle burst */
void showplayer(ENetEvent& event, const std::string_view text)
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

    const bool was_hidden = (pPeer->state & S_INVISIBLE) == S_INVISIBLE;
    if (!was_hidden)
    {
        on::ConsoleMessage(event.peer, "`oYou're already visible.``");
        return;
    }
    pPeer->state &= ~(S_INVISIBLE | S_GHOST); // @note /invis also turned on ghost - turn both off

    // put the sprite back for everyone else
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [&event, pPeer](ENetPeer& peer)
    {
        ::peer *pOthers = static_cast<::peer*>(peer.data);
        if (pOthers->user_id == pPeer->user_id)
        {
            on::SetClothing(peer);
            return;
        }

        on::Spawn(peer, pPeer->netid, pPeer->user_id, pPeer->pos,
                  pPeer->display_growid, pPeer->country,
                  pPeer->role, pPeer->role >= DEVELOPER, false);
        on::SetClothing(peer, *pPeer); // @note clothes + normal colour, otherwise they show up grey
        send_varlist(&peer, { "OnSetPos", CL_Vec2f{ pPeer->pos.x, pPeer->pos.y } }, pPeer->netid);
    });

    // burst of particles at your feet, seen by the whole world
    const ::pos burst = pPeer->pos;
    // three rings at increasing radius, offset so they interleave
    const float radii[3] = { 40.0f, 70.0f, 100.0f };
    for (int r = 0; r < 3; ++r)
    {
        const int count = 10 + r * 4;
        for (int i = 0; i < count; ++i)
        {
            const float angle = (3.14159265f * 2.0f) * (static_cast<float>(i) / count)
                              + (r * 0.35f);
            const ::pos at{ burst.x + std::cos(angle) * radii[r],
                            burst.y + std::sin(angle) * radii[r] };
            send_particle_effect(event, at, { 0x0e, 0x03 });
        }
    }
    send_particle_effect(event, burst, { 0x0e, 0x03 });

    // mod-reveal sound, heard by everyone in the world
    peers(pPeer->recent_worlds.back(), PEER_SAME_WORLD, [pPeer](ENetPeer &p)
    {
        send_varlist(&p, { "OnPlayPositioned", "audio/magic.wav" }, pPeer->netid);
    });

    on::ConsoleMessage(event.peer, "`2You reappear in a shower of sparks.``");
}
