#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "sphere.hpp"

/* /sphere {itemID} {radius} -> draws a filled circle of blocks around you */
void sphere(ENetEvent& event, const std::string_view text)
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

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    std::string id_str{}, radius_str{};
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos)
    {
        id_str = arg.substr(0, sp);
        radius_str = arg.substr(sp + 1);
    }
    else
    {
        on::ConsoleMessage(event.peer, "`4Usage: /sphere {itemID} {radius}``");
        return;
    }

    auto digits = [](std::string s){ std::erase_if(s, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)); }); return s; };
    id_str = digits(id_str);
    radius_str = digits(radius_str);

    if (id_str.empty() || radius_str.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /sphere {itemID} {radius}``");
        return;
    }

    signed id{}, radius{};
    try { id = std::stoi(id_str); radius = std::stoi(radius_str); }
    catch (...) { on::ConsoleMessage(event.peer, "`4Those are not valid numbers.``"); return; }

    radius = std::clamp(radius, 1, 25);

    const ::pos tile = pPeer->pos.by_32(true);
    const signed cx = static_cast<signed>(tile.x);
    const signed cy = static_cast<signed>(tile.y);

    std::size_t changed = 0;
    for (signed dy = -radius; dy <= radius; ++dy)
    {
        for (signed dx = -radius; dx <= radius; ++dx)
        {
            if (dx * dx + dy * dy > radius * radius) continue;

            const signed x = cx + dx;
            const signed y = cy + dy;
            if (x < 0 || x >= 100 || y < 0 || y >= 60) continue;

            ::block &b = world->blocks[cord(x, y)];
            if (b.fg == 6 || b.fg == 8) continue;

            b.fg = static_cast<short>(id);
            send_tile_update(event, { .id = static_cast<short>(id), .punch = ::pos{ x, y } }, b, *world);
            ++changed;
        }
    }

    on::ConsoleMessage(event.peer,
        std::format("`2Placed `w{}`` blocks in a radius of `w{}``.``", changed, radius));
}
