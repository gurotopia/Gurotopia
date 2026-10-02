#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "setspawn.hpp"

void setspawn(ENetEvent& event, const std::string_view text)
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

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    world->spawn = pPeer->pos;
    pPeer->rest_pos = pPeer->pos;

    const ::pos tile = pPeer->pos.by_32(true);
    on::ConsoleMessage(event.peer,
        std::format("`2Spawn set to `w{}``, `w{}``.``",
                    static_cast<signed>(tile.x), static_cast<signed>(tile.y)));
}
