#include "pch.hpp"
#include "action/join_request.hpp"
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
        case std::byte{ type::DOOR }: // @todo add door-to-door with door::id
        case std::byte{ type::PORTAL }:
        {
            for (::door &door : w->second.doors)
            {
                if (door.pos == state.punch) 
                {
                    if (door.dest.empty()) break;
                    const std::string_view world_name{ door.dest };
                    
                    quit_to_exit(event, "", true);
                    join_request(event, "", world_name);
                }
            }
            break;
        }
    }

    state_visuals(event, std::move(state)); // finished.
}