#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "dropqueue.hpp"
#include "clear.hpp"

/* /clear -> removes every dropped item lying on the ground (everyone sees it, nobody has to rejoin) */
void clear_cmd(ENetEvent& event, const std::string_view text)
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

    const std::size_t count = world->objects.size();
    if (count == 0)
    {
        on::ConsoleMessage(event.peer, "`4There is nothing on the ground here.``");
        return;
    }

    dropqueue_cancel_adds(world->name);
    for (const ::object &o : world->objects)
        queue_remove_drop(world->name, static_cast<int>(o.uid), pPeer->growid);

    on::ConsoleMessage(event.peer, std::format("`2Clearing `w{}`` dropped item{}...``", count, count == 1 ? "" : "s"));
}
