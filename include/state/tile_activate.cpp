#include "pch.hpp"
#include "action/quit_to_exit.hpp"
#include "tile_activate.hpp"

void tile_activate(ENetEvent& event, state state)
{
    auto &peer = _peer[event.peer];
    auto w = worlds.find(peer->recent_worlds.back());
    if (w == worlds.end()) return;

    block &block = w->second.blocks[cord(state.punch[0], state.punch[1])];
    item &item = items[block.fg]; // @todo handle bg

    switch (item.type)
    {
        case std::byte{ type::MAIN_DOOR }:
        {
            quit_to_exit(event, "", false);
            break;
        }
    }

    state_visuals(event, std::move(state)); // finished.
}