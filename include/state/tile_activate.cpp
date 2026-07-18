#include "pch.hpp"
#include "action/join_request.hpp"
#include "action/quit_to_exit.hpp"
#include "tile_activate.hpp"

void tile_activate(ENetEvent& event, state state)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    ::block &block = world->blocks[cord(state.punch.x, state.punch.y)];
    const ::item &item = id_to_item(block.fg);
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
                send_varlist(event.peer, {
                    "OnSetPos", 
                    CL_Vec2f{pPeer->rest_pos.x, pPeer->rest_pos.y}
                }, pPeer->netid);
                send_varlist(event.peer, {
                    "OnZoomCamera",
                    CL_Vec2f{10000.0f, 0}, // @todo
                    1000u
                });
                send_varlist(event.peer, {
                    "OnSetFreezeState", 
                    0u
                }, pPeer->netid);
                
                // audio/teleport.wav
            }
            break;
        }
        case type::CHECKPOINT:
        {
            ::block &checkpoint = world->blocks[cord(pPeer->rest_pos.by_32(true).x, pPeer->rest_pos.by_32(true).y)]; // @note get previous checkpoint from respawn position

            checkpoint.state[2] &= ~S_TOGGLE;
            send_tile_update(event, ::state{.id = block.fg/*has to be 'block' or else iterfere with main door*/, .punch = pPeer->rest_pos.by_32(true)}, checkpoint, *world);

            pPeer->rest_pos = state.punch.by_32();
            block.state[2] |= S_TOGGLE; // @note toggle current checkpoint
            send_tile_update(event, ::state{.id = block.fg, .punch = state.punch}, block, *world);
            break;
        }
    }

    state_visuals(*event.peer, std::move(state)); // finished.
}