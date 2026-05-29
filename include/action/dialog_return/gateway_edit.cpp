#include "pch.hpp"

#include "gateway_edit.hpp"

void gateway_edit(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    const short tilex = atoi(pipes[5zu].c_str());
    const short tiley = atoi(pipes[8zu].c_str());

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    block &block = world->blocks[cord(tilex, tiley)];

    if (pipes[3zu] == "door_edit" || pipes[3zu] == "sign_edit") 
        block.label = pipes[11zu];
        
    else if (pipes[3zu] == "gateway_edit") 
    {
        block.state[2] &= ~(S_PUBLIC | S_LOCKED);
        block.state[2] |= stoi(pipes[11zu]) ? S_PUBLIC : S_LOCKED;
    }

    send_tile_update(event, {
        .id = block.fg,
        .punch = { tilex, tiley }
    }, block, *world);

    if (pipes[10zu] == "door_name" && pipes.size() > 12zu)
    {
        for (::door &door : world->doors)
        {
            if (door.pos == ::pos{tilex, tiley}) 
            {
                door.dest = pipes[13];
                door.id = pipes[15];
                return;
            }
        }
        world->doors.emplace_back(::door(
            pipes[13],
            pipes[15],
            "", // @todo add password door
            { tilex, tiley }
        ));
    }
}