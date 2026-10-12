#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include "trash_item.hpp"

void trash_item(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    const short itemID = atoi(hPipe["itemID"].c_str());

    const ::item &item = id_to_item(itemID);
    if (item.type == type::FIST || item.type == type::WRENCH) return;

    auto slot = std::ranges::find(pPeer->slots, itemID, &::slot::id);
    if (slot == pPeer->slots.end()) return; // @note you can only recycle what you have
    const short count = static_cast<short>(std::clamp(atoi(hPipe["count"].c_str()), 0, static_cast<int>(slot->count)));
    if (count == 0) return;

    modify_item_inventory(event, ::slot(itemID, -count));
    on::ConsoleMessage(event.peer, std::format("{} `w{}`` recycled, `w0`` gems earned.", count, item.raw_name));
}