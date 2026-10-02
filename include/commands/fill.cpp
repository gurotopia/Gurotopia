#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "worldreload.hpp"
#include "database/world.hpp"
#include "fill.hpp"

/* /fill {itemID} -> fills every empty tile in the world with that block */
void fill(ENetEvent& event, const std::string_view text)
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

    std::erase_if(arg, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)); });
    if (arg.empty())
    {
        on::ConsoleMessage(event.peer, "`4Usage: /fill {itemID}``");
        return;
    }

    signed id{};
    try { id = std::stoi(arg); }
    catch (...) { on::ConsoleMessage(event.peer, "`4That is not a valid item id.``"); return; }

    std::size_t changed = 0;
    for (std::size_t i = 0ull; i < world->blocks.size(); ++i)
    {
        ::block &b = world->blocks[i];
        if (b.fg == 6 || b.fg == 8) continue;  // keep main door + bedrock
        if (b.fg != 0) continue;               // only fill empty space
        b.fg = static_cast<short>(id);
        ++changed;
    }

    const std::string world_name = world->name;
    on::ConsoleMessage(event.peer, std::format("`2Filled `w{}`` tiles with item `w{}``.``", changed, id));

    reload_world_all(world_name); // @note everyone in the world, not just you
}
