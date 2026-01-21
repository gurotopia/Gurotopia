#include "pch.hpp"

#include "drop_item.hpp"

void drop_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    const short id = atoi(pipes[5zu].c_str());
    short count = atoi(pipes[8zu].c_str());

    for (const ::slot &slot : peer->slots)
        if (slot.id == id)
            if (count > slot.count) count = slot.count;
            else if (count < 0) count = 0;

    modify_item_inventory(event, ::slot(id, -count));

    float x_nabor = (peer->facing_left) ? peer->pos.f_x() - 32 : peer->pos.f_x() + 32; // @note peer's naboring tile (drop position)
    item_change_object(event, {id, count}, ::pos{x_nabor, peer->pos.f_y()});
}