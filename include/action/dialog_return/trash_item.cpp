#include "pch.hpp"

#include "trash_item.hpp"

void trash_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    const short id = atoi(pipes[5zu].c_str());
    short count = atoi(pipes[8zu].c_str());

    auto item = std::ranges::find(items, id, &::item::id);

    for (const ::slot &slot : peer->slots)
        if (slot.id == id)
            if (count > slot.count) count = slot.count;
            else if (count < 0) count = 0;

    modify_item_inventory(event, ::slot(id, -count));

    packet::create(*event.peer, false, 0, {
        "OnConsoleMessage",
        std::format("{} `w{}`` recycled, `w0`` gems earned.", count, item->raw_name).c_str()
    });
}