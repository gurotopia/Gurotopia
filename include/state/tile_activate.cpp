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
            action::quit_to_exit(event, "", false);
            break;
        }
        case std::byte{ type::DOOR }: // @todo add door-to-door with door::id
        case std::byte{ type::PORTAL }:
        {
            bool has_dest{ false };
            for (::door &door : w->second.doors)
            {
                if (door.pos == state.punch) 
                {
                    has_dest = true;
                    const std::string_view world_name{ door.dest };
                    
                    action::quit_to_exit(event, "", true);
                    action::join_request(event, "", world_name);
                    break;
                }
            }
            if (!has_dest)
            {
                packet::create(*event.peer, true, 0, {
                    "OnSetPos", 
                    std::vector<float>{peer->rest_pos.front(), peer->rest_pos.back()}
                });
                packet::create(*event.peer, false, 0, {
                    "OnZoomCamera", 
                    std::vector<float>{10000.0f}, // @todo
                    1000u
                });
                packet::create(*event.peer, true, 0, { "OnSetFreezeState", 0u });
                packet::create(*event.peer, true, 0, { "OnPlayPositioned", "audio/teleport.wav" });
            }
            break;
        }
    }

    state_visuals(event, std::move(state)); // finished.
}