#include "pch.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "tile_activate.hpp"

void tile_activate(ENetEvent& event, state state)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    if (!worlds.contains(peer->recent_worlds.back())) return;
    ::world &world = worlds.at(peer->recent_worlds.back());

    ::block &block = world.blocks[cord(state.punch.x, state.punch.y)];
    ::item &item = items[block.fg]; // @todo handle bg

    switch (item.type)
    {
        case type::MAIN_DOOR:
        {
            action::quit_to_exit(event, "", false);
            break;
        }
        case type::DOOR: // @todo add door-to-door with door::id
        case type::PORTAL:
        {
            bool has_dest{};
            for (::door &door : world.doors)
            {
                if (door.pos == state.punch) 
                {
                    has_dest = true;
                    const std::string_view dest{ door.dest };
                    
                    action::quit_to_exit(event, "", true);
                    action::join_request(event, "", dest);
                    break;
                }
            }
            if (!has_dest)
            {
                packet::create(*event.peer, true, 0, {
                    "OnSetPos", 
                    std::vector<float>{peer->rest_pos.f_x(), peer->rest_pos.f_y()}
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

    state_visuals(*event.peer, std::move(state)); // finished.
}