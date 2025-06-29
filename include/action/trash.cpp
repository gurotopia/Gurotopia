#include "pch.hpp"
#include "tools/string.hpp"
#include "trash.hpp"

void trash(ENetEvent& event, const std::string& header)
{
    std::string itemID = readch(std::move(header), '|')[4];
    if (itemID.empty()) return;

    short id = stoi(itemID);
    item &item = items[id];

    if (item.cat == std::byte{ 0x80 })
    {
        gt_packet(*event.peer, false, 0, { "OnTextOverlay", "You'd be sorry if you lost that!" });
        return;
    }

    for (const slot &slot : _peer[event.peer]->slots)
        if (slot.id == id)
        {
            gt_packet(*event.peer, false, 0, {
                "OnDialogRequest",
                std::format(
                    "set_default_color|`o\n"
                    "add_label_with_icon|big|`4Recycle`` `w{0}``|left|{1}|\n"
                    "add_label|small|You will get 0 gems per item.|left|\n" // @todo get rgt gem amount for trashing
                    "add_textbox|How many to `4destroy``? (you have {2})|left|\n"
                    "add_text_input|count||0|5|\n"
                    "embed_data|itemID|{1}\n"
                    "end_dialog|trash_item|Cancel|OK|\n",
                    item.raw_name, slot.id, slot.count
                ).c_str()
            });
            return;
        }
}