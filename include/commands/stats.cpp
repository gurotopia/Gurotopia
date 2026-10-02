#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "stats.hpp"

/* /stats -> server and world numbers */
void stats(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Only staff can use this.``");
        return;
    }

    const std::size_t online = peers().size();
    const std::size_t loaded = worlds.size();

    on::ConsoleMessage(event.peer, "`o--- `2Server stats`o ---");
    on::ConsoleMessage(event.peer, std::format("`oPlayers online: `w{}``", online));
    on::ConsoleMessage(event.peer, std::format("`oWorlds loaded: `w{}``", loaded));

    if (pPeer->netid != 0)
    {
        auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
        if (world != worlds.end())
        {
            std::size_t solid = 0, background = 0;
            for (const ::block &b : world->blocks)
            {
                if (b.fg != 0) ++solid;
                if (b.bg != 0) ++background;
            }

            on::ConsoleMessage(event.peer, std::format("`o--- `2{}`o ---", world->name));
            on::ConsoleMessage(event.peer, std::format("`oBlocks: `w{}`` foreground, `w{}`` background", solid, background));
            on::ConsoleMessage(event.peer, std::format("`oVisitors here: `w{}``", world->visitors));
            on::ConsoleMessage(event.peer, std::format("`oOwner id: `w{}``", world->owner));
            on::ConsoleMessage(event.peer, std::format("`oPublic: `w{}``", world->is_public ? "yes" : "no"));
            on::ConsoleMessage(event.peer, std::format("`oNuked: `w{}``", world->nuked ? "yes" : "no"));
        }
    }
}
