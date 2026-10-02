#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "worldreload.hpp"
#include "database/world.hpp"
#include "reset.hpp"

void reset(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < DEVELOPER)
    {
        on::ConsoleMessage(event.peer, "`4Only developers can reset a world.``");
        return;
    }

    if (pPeer->netid == 0)
    {
        on::ConsoleMessage(event.peer, "`4You must be in a world to reset it.``");
        return;
    }

    const std::string world_name = pPeer->recent_worlds.back();

    auto world = std::ranges::find(worlds, world_name, &::world::name);
    if (world == worlds.end())
    {
        on::ConsoleMessage(event.peer, "`4World not found.``");
        return;
    }

    // wipe the world back to a freshly generated state
    world->blocks.clear();
    world->objects.clear();
    world->signs.clear();
    world->trees.clear();
    world->owner = 0;
    world->access.fill(0);
    world->last_object_uid = 0;
    generate_world(*world);

    on::ConsoleMessage(event.peer, std::format("`2World `w{}`` has been reset.``", world_name));

    // everyone in the world re-enters so they all receive the new map
    reload_world_all(world_name);
}
