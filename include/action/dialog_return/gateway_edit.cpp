#include "pch.hpp"

#include "gateway_edit.hpp"

void gateway_edit(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    const short tilex = atoi(pipes[5zu].c_str());
    const short tiley = atoi(pipes[8zu].c_str());

    auto it = worlds.find(_peer[event.peer]->recent_worlds.back());
    if (it == worlds.end()) return;

    block &block = it->second.blocks[cord(tilex, tiley)];

    if (pipes[3zu] == "door_edit" || pipes[3zu] == "sign_edit") block.label = pipes[11zu];
    else if (pipes[3zu] == "gateway_edit") block._public = stoi(pipes[11zu]);

    tile_update(event, {
        .id = block.fg,
        .pos = { tilex * 32.0f, tiley * 32.0f },
        .punch = { tilex, tiley }
    }, block, it->second);

    if (pipes[10zu] == "door_name" && pipes.size() > 12zu)
    {
        for (::door& door : it->second.doors)
        {
            if (door.pos == std::array<int, 2ULL>{ tilex, tiley }) 
            {
                door.dest = pipes[13];
                door.id = pipes[15];
                return;
            }
        }
        it->second.doors.emplace_back(door(
            pipes[13],
            pipes[15],
            "", // @todo add password door
            { tilex, tiley }
        ));
    }
}