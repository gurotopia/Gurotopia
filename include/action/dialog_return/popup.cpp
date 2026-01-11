#include "pch.hpp"

#include "tools/string.hpp" // @note to_char()

#include "popup.hpp"

void popup(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    auto &peer = _peer[event.peer];
    if (pipes.size() <= 11) return; // @note "Continue" botton on wrench has no data. so we return early.

    if (pipes[11zu] == "my_worlds")
    {
        auto section = [](const auto& range) 
        {
            std::string result;
            for (const auto &name : range)
                if (!name.empty())
                    result.append(std::format("add_button|{0}|{0}|noflags|0|0|\n", name));
            return result;
        };
        packet::create(*event.peer, false, 0, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
                "start_custom_tabs|\n"
                "add_custom_button|myWorldsUiTab_0|image:interface/large/btn_tabs2.rttex;image_size:228,92;frame:1,0;width:0.15;|\n"
                "add_custom_button|myWorldsUiTab_1|image:interface/large/btn_tabs2.rttex;image_size:228,92;frame:0,1;width:0.15;|\n"
                "add_custom_button|myWorldsUiTab_2|image:interface/large/btn_tabs2.rttex;image_size:228,92;frame:0,2;width:0.15;|\n"
                "end_custom_tabs|\n"
                "add_label|big|Locked Worlds|left|0|\n"
                "add_spacer|small|\n"
                "add_textbox|Place a World Lock in a world to lock it. Break your World Lock to unlock a world.|left|\n"
                "add_spacer|small|\n"
                "{}\n"
                "add_spacer|small|\n"
                "end_dialog|worlds_list||Back|\n"
                "add_quick_exit|\n",
                section(peer->my_worlds)
            ).c_str()
        });
    }
    else if (pipes[11zu] == "billboard_edit")
    {
        packet::create(*event.peer, false, 0, {
            "OnDialogRequest",
            std::format("set_default_color|`o\n"
            "add_label_with_icon|big|`wTrade Billboard``|left|8282|\n"
            "add_spacer|small|\n"
            "{}"
            "add_item_picker|billboard_item|`wSelect Billboard Item``|Choose an item to put on your billboard!|\n"
            "add_spacer|small|\n"
            "add_checkbox|billboard_toggle|`$Show Billboard``|{}\n"
            "add_checkbox|billboard_buying_toggle|`$Is Buying``|{}\n"
            "add_text_input|setprice|Price of item:|{}|5|\n"
            "add_checkbox|chk_peritem|World Locks per Item|{}\n"
            "add_checkbox|chk_perlock|Items per World Lock|{}\n"
            "add_spacer|small|\n"
            "end_dialog|billboard_edit|Close|Update|\n",
            (peer->billboard.id == 0) ? 
                "" : 
                std::format("add_label_with_icon|small|`w{}``|left|{}|\n", items[peer->billboard.id].raw_name, peer->billboard.id),
            to_char(peer->billboard.show),
            to_char(peer->billboard.isBuying),
            peer->billboard.price,
            to_char(peer->billboard.perItem),
            to_char(!peer->billboard.perItem)
            ).c_str()
        });
    }
}