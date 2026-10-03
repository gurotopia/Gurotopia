#include "pch.hpp"

#include "gateway_edit.hpp"

void gateway_edit(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    const short tilex = atoi(hPipe["tilex"].c_str());
    const short tiley = atoi(hPipe["tiley"].c_str());

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    if (tilex < 0 || tilex >= 100 || tiley < 0 || cord(tilex, tiley) >= world->blocks.size()) return;
    if (world->owner != 0 && world->owner != pPeer->user_id && std::ranges::find(world->access, pPeer->user_id) == world->access.end()) return; // @note same rule as building here

    block &block = world->blocks[cord(tilex, tiley)];

    if (hPipe["dialog_name"] == "sign_edit") 
    {
        auto sign = std::ranges::find(world->signs, ::pos{tilex, tiley}, &::sign::pos);
        if (sign != world->signs.end()) 
        {
            sign->label = hPipe["sign_text"];
        }
        else {
            world->signs.emplace_back(::sign(hPipe["sign_text"], ::pos{tilex, tiley}));
        }
    }
    else if (hPipe["dialog_name"] == "door_edit") 
    {
        auto door = std::ranges::find(world->doors, ::pos{tilex, tiley}, &::door::pos);
        if (door != world->doors.end())
        {
            door->label = hPipe["door_name"];
            if (!hPipe["dialog_name"].empty())
            {
                door->dest = hPipe["door_target"];
                door->id = hPipe["door_id"];
            }
        }
        else {
            world->doors.emplace_back(::door(
                hPipe["door_name"],
                hPipe["door_target"],
                hPipe["door_id"],
                { tilex, tiley }
            ));
        }
    }
    else if (hPipe["dialog_name"] == "gateway_edit") 
    {
        block.state[2] &= ~(S_PUBLIC | S_LOCKED);
        block.state[2] |= atoi(hPipe["checkbox_public"].c_str()) ? S_PUBLIC : S_LOCKED;
    }

    send_tile_update(event, {
        .id = block.fg,
        .punch = { tilex, tiley }
    }, block, *world);
}