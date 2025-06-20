#include "pch.hpp"
#include "network/packet.hpp"
#include "drop.hpp"

#include "tools/string_view.hpp"

void drop(ENetEvent event, const std::string& header)
{
    std::string itemID = readch(std::move(header), '|')[4];
    if (itemID.empty()) return;

    short id = stoi(itemID);
    item &item = items[id];

    if (item.cat == std::byte{ 0x80 })
    {
        gt_packet(*event.peer, false, 0, { "OnTextOverlay", "You can't drop that." });
        return;
    }
    
    for (const slot &slot : _peer[event.peer]->slots)
        if (slot.id == id) 
        {
            gt_packet(*event.peer, false, 0, {
                "OnDialogRequest", 
                std::format(
                    "set_default_color|`o\n"
                    "add_label_with_icon|big|`wDrop {0}``|left|{1}|\n"
                    "add_textbox|How many to drop?|left|\n"
                    "add_text_input|count||{2}|5|\n"
                    "embed_data|itemID|{1}\n"
                    "end_dialog|drop_item|Cancel|OK|\n", 
                    item.raw_name, id, slot.count
                ).c_str()
            });
            return;
        }
}