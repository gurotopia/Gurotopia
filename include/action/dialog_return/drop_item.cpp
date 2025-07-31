#include "pch.hpp"

#include "drop_item.hpp"

void drop_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    auto &peer = _peer[event.peer];

    const short id = atoi(pipes[5zu].c_str());
    const short count = atoi(pipes[8zu].c_str());

    peer->emplace(slot(id, -count)); // @note take away
    inventory_visuals(event);

    float x_nabor = (peer->facing_left ? peer->pos[0] - 1 : peer->pos[0] + 1); // @note peer's naboring tile (drop position)
    item_change_object(event, {id, count}, {x_nabor, peer->pos[1]});
}