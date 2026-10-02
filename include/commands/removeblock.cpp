#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "removeblock.hpp"

/* /removeblock -> clears the tile you are standing on */
void removeblock(ENetEvent& event, const std::string_view text)
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

    const ::pos tile = pPeer->pos.by_32(true);
    const signed x = static_cast<signed>(tile.x);
    const signed y = static_cast<signed>(tile.y);

    if (x < 0 || x >= 100 || y < 0 || y >= 60)
    {
        on::ConsoleMessage(event.peer, "`4That is outside the world.``");
        return;
    }

    ::block &block = world->blocks[cord(x, y)];
    const auto removed_fg = block.fg;
    const auto removed_bg = block.bg;

    if (removed_fg == 0 && removed_bg == 0)
    {
        on::ConsoleMessage(event.peer, std::format("`4Nothing at `w{}``, `w{}``.``", x, y));
        return;
    }

    block.reset();

    send_tile_update(event, {
        .id = 0,
        .punch = ::pos{ x, y }
    }, block, *world);

    on::ConsoleMessage(event.peer,
        std::format("`2Removed block at `w{}``, `w{}`` (fg `w{}``, bg `w{}``).``",
                    x, y, removed_fg, removed_bg));
}
