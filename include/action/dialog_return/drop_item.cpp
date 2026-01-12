#include "pch.hpp"

#include "drop_item.hpp"

void drop_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    auto &peer = _peer[event.peer];

    const short id = atoi(pipes[5zu].c_str());
    short count = atoi(pipes[8zu].c_str());

    for (const ::slot &slot : _peer[event.peer]->slots)
        if (slot.id == id)
            if (count > slot.count) count = slot.count;
            else if (count < 0) count = 0;

    modify_item_inventory(event, {id, -count});

    int x_nabor = (peer->facing_left) ? (peer->pos[0]/32) - 1 : (peer->pos[0]/32) + 1; // @note peer's naboring tile (drop position)
    add_drop(event, {id, count}, ::pos{x_nabor, static_cast<int>(peer->pos[1]/32)});
}