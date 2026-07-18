#include "pch.hpp"
#include "on/ConsoleMessage.hpp"

#include "trash_item.hpp"

void trash_item(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    const short itemID = atoi(hPipe["itemID"].c_str());
    short count = atoi(hPipe["count"].c_str());

    const ::item &item = id_to_item(itemID);

    for (const ::slot &slot : pPeer->slots)
        if (slot.id == itemID)
            if (count > slot.count) count = slot.count;
            else if (count < 0) count = 0;

    modify_item_inventory(event, ::slot(itemID, -count));
    on::ConsoleMessage(event.peer, std::format("{} `w{}`` recycled, `w0`` gems earned.", count, item.raw_name));
}