#include "pch.hpp"

#include "trash_item.hpp"

void trash_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    const short id = atoi(pipes[5zu].c_str());
    const short count = atoi(pipes[8zu].c_str());

    _peer[event.peer]->emplace(slot(id, -count)); // @note take away
    inventory_visuals(event);

    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage",
        std::format("{} `w{}`` recycled, `w0`` gems earned.", count, items[id].raw_name).c_str()
    });
}