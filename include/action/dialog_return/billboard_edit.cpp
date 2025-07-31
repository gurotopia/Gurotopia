#include "pch.hpp"

#include "on/BillboardChange.hpp"

#include "billboard_edit.hpp"

void billboard_edit(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    auto &peer = _peer[event.peer];

    if (pipes[4zu] == "billboard_item") 
    {
        const short billboard_item = atoi(pipes[5zu].c_str());

        if (billboard_item == 18 || billboard_item == 32) return;

        peer->billboard = {
            .id = billboard_item,
            .show = stoi(pipes[7zu]) != 0,
            .isBuying = stoi(pipes[9zu]) != 0,
            .price = stoi(pipes[11zu]),
            .perItem = stoi(pipes[13zu]) != 0,
        };
    }
    else // @note billboard_toggle
    {
        peer->billboard = {
            .id = peer->billboard.id,
            .show = stoi(pipes[5zu]) != 0,
            .isBuying = stoi(pipes[7zu]) != 0,
            .price = stoi(pipes[9zu]),
            .perItem = stoi(pipes[11zu]) != 0,
        };
    }
    on::BillboardChange(event);
}