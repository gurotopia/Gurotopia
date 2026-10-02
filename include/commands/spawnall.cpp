#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "spawnall.hpp"

/* /spawnall -> sends everyone in the world back to the spawn door */
void spawnall(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Only staff can use this.``");
        return;
    }

    if (pPeer->netid == 0)
    {
        on::ConsoleMessage(event.peer, "`4You must be in a world.``");
        return;
    }

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    const ::pos spawn = world->spawn;

    std::size_t moved = 0;
    peers(world->name, PEER_SAME_WORLD, [&](ENetPeer &p)
    {
        ::peer *pTarget = static_cast<::peer*>(p.data);
        if (!pTarget) return;
        if (pTarget->role >= MODERATOR) return; // @note mods and devs stay where they are

        pTarget->pos = spawn;
        send_varlist(&p, { "OnSetPos", CL_Vec2f{ spawn.x, spawn.y } }, pTarget->netid);
        ++moved;
    });

    on::ConsoleMessage(event.peer, std::format("`2Sent `w{}`` player{} to spawn.``", moved, moved == 1 ? "" : "s"));
}
