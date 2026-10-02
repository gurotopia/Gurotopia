#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "undo.hpp"

/* /undo        -> reverts your last block change
 * /undo {n}    -> reverts the last n changes (max 50)
 */
void undo(ENetEvent& event, const std::string_view text)
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

    printf("[undo] history size=%zu world=%s\n", pPeer->block_history.size(), world->name.c_str());
    if (pPeer->block_history.empty())
    {
        on::ConsoleMessage(event.peer, "`4Nothing to undo.``");
        return;
    }

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    signed count = 1;
    if (!arg.empty())
    {
        std::erase_if(arg, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)); });
        if (!arg.empty())
        {
            try { count = std::stoi(arg); } catch (...) { count = 1; }
        }
    }
    count = std::clamp(count, 1, 50);

    std::size_t reverted = 0;
    while (reverted < static_cast<std::size_t>(count) && !pPeer->block_history.empty())
    {
        const ::block_change change = pPeer->block_history.back();
        pPeer->block_history.pop_back();

        if (change.world != world->name) continue; // only undo in the world it happened

        if (change.x < 0 || change.x >= 100 || change.y < 0 || change.y >= 60) continue;

        ::block &b = world->blocks[cord(change.x, change.y)];
        b.fg = change.old_fg;
        b.bg = change.old_bg;

        send_tile_update(event, { .id = b.fg, .punch = ::pos{ change.x, change.y } }, b, *world);
        ++reverted;
    }

    on::ConsoleMessage(event.peer,
        std::format("`2Reverted `w{}`` block change{}.``", reverted, reverted == 1 ? "" : "s"));
}
