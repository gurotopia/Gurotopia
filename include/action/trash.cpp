#include "pch.hpp"
#include "database/items.hpp"
#include "database/peer.hpp"
#include "network/packet.hpp"
#include "trash.hpp"

#include "tools/string_view.hpp"

void trash(ENetEvent event, const std::string& header)
{
    for (const slot &slot : _peer[event.peer]->slots)
        if (slot.id == stoi(readch(std::string{header}, '|')[4]))
        {
            item &item = items[slot.id];
            if (item.type == std::byte{ type::WRENCH } || item.type == std::byte{ type::FIST }) return;
            gt_packet(*event.peer, false, 0, {
                "OnDialogRequest",
                std::format("set_default_color|`o\n"
                "add_label_with_icon|big|`4Recycle`` `w{0}``|left|{1}|\n"
                "add_label|small|You will get 0 gems per item.|left|\n" // @todo get rgt gem amount for trashing
                "add_textbox|How many to `4destroy``? (you have {2})|left|\n"
                "add_text_input|count||0|5|\n"
                "embed_data|itemID|{1}\n"
                "end_dialog|trash_item|Cancel|OK|\n",
                item.raw_name, slot.id, slot.count).c_str()
            });
            return;
        }
}