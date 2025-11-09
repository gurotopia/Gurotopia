#include "pch.hpp"

#include "trash_item.hpp"

void trash_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    const short id = atoi(pipes[5zu].c_str());
    short count = atoi(pipes[8zu].c_str());

    for (const ::slot &slot : _peer[event.peer]->slots)
        if (slot.id == id)
            if (count > slot.count) count = slot.count;
            else if (count < 0) count = 0;

    _peer[event.peer]->emplace(slot(id, -count)); // @note take away
    modify_item_inventory(event, {id, count});

    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage",
        std::format("{} `w{}`` recycled, `w0`` gems earned.", count, items[id].raw_name).c_str()
    });
}