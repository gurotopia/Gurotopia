#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "box.hpp"

/* /box {itemID} {width} {height} -> draws a rectangle with you at the top-left */
void box(ENetEvent& event, const std::string_view text)
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

    std::vector<std::string> parts;
    {
        std::size_t start = 0;
        while (start < arg.size())
        {
            const std::size_t sp = arg.find(' ', start);
            if (sp == std::string::npos) { parts.push_back(arg.substr(start)); break; }
            parts.push_back(arg.substr(start, sp - start));
            start = sp + 1;
        }
    }

    if (parts.size() < 3)
    {
        on::ConsoleMessage(event.peer, "`4Usage: /box {itemID} {width} {height}``");
        return;
    }

    auto digits = [](std::string s){ std::erase_if(s, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)); }); return s; };

    signed id{}, w{}, h{};
    try
    {
        id = std::stoi(digits(parts[0]));
        w  = std::stoi(digits(parts[1]));
        h  = std::stoi(digits(parts[2]));
    }
    catch (...) { on::ConsoleMessage(event.peer, "`4Those are not valid numbers.``"); return; }

    w = std::clamp(w, 1, 60);
    h = std::clamp(h, 1, 40);

    const ::pos tile = pPeer->pos.by_32(true);
    const signed ox = static_cast<signed>(tile.x);
    const signed oy = static_cast<signed>(tile.y);

    std::size_t changed = 0;
    for (signed dy = 0; dy < h; ++dy)
    {
        for (signed dx = 0; dx < w; ++dx)
        {
            const signed x = ox + dx;
            const signed y = oy + dy;
            if (x < 0 || x >= 100 || y < 0 || y >= 60) continue;

            ::block &b = world->blocks[cord(x, y)];
            if (b.fg == 6 || b.fg == 8) continue;

            b.fg = static_cast<short>(id);
            send_tile_update(event, { .id = static_cast<short>(id), .punch = ::pos{ x, y } }, b, *world);
            ++changed;
        }
    }

    on::ConsoleMessage(event.peer,
        std::format("`2Placed `w{}`` blocks in a `w{}``x`w{}`` box.``", changed, w, h));
}
