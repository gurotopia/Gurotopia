#include "pch.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "tile_activate.hpp"

void tile_activate(ENetEvent& event, state state)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    auto world = std::ranges::find(worlds, peer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    ::block &block = world->blocks[cord(state.punch.x, state.punch.y)];
    auto item = std::ranges::find(items, block.fg, &::item::id);

    switch (item->type)
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
            for (::door &door : world->doors)
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
        case type::CHECKPOINT:
        {
            ::block &checkpoint = world->blocks[cord(peer->rest_pos.x, peer->rest_pos.y)]; // @note get previous checkpoint from respawn position

            checkpoint.state3 &= ~S_TOGGLE;
            send_tile_update(event, ::state{.id = block.fg/*has to be 'block' or else iterfere with main door*/, .punch = peer->rest_pos}, checkpoint, *world);

            peer->rest_pos = state.punch;
            block.state3 |= S_TOGGLE; // @note toggle current checkpoint
            send_tile_update(event, ::state{.id = block.fg, .punch = state.punch}, block, *world);
            break;
        }
    }

    state_visuals(*event.peer, std::move(state)); // finished.
}