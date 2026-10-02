#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "unnuke.hpp"

/* /unnuke -> reopens the world you are standing in */
void unnuke(ENetEvent& event, const std::string_view text)
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

    if (!world->nuked)
    {
        on::ConsoleMessage(event.peer, std::format("`4`w{}`` is not nuked.``", world->name));
        return;
    }

    world->nuked = false;

    peers(world->name, PEER_SAME_WORLD, [&world](ENetPeer &p)
    {
        on::ConsoleMessage(&p, std::format("`5[```2{}`` `5is open again!```5]``", world->name));
    });
}
