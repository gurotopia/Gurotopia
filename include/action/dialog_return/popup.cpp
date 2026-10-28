#include "pch.hpp"

#include "popup.hpp"
#include <unordered_map>

static const std::unordered_map<std::string, std::string> rml_pages{
    {"seed_diary_customization", "show_seed_diary_ui"},
    {"open_worldlock_storage", "show_world_lock_storage"}
};

void popup(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (hPipe["buttonClicked"] == "my_worlds")
    {
        auto section = [](const auto &range) 
        {
            std::string result;
            for (const std::string &name : range)
                if (!name.empty())
                    result.append(std::format("add_button|{0}|{0}|noflags|0|0|\n", name));
            return result;
        };
        send_varlist(event.peer, {
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
                section(pPeer->my_worlds)
            )
        });
    }
    else if (hPipe["buttonClicked"] == "billboard_edit")
    {
        const ::item &item = id_to_item(pPeer->billboard.id);

        send_varlist(event.peer, {
            "OnDialogRequest",
            std::format(
                "set_default_color|`o\n"
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
                (pPeer->billboard.id == 0) ? 
                    "" : 
                    std::format("add_label_with_icon|small|`w{}``|left|{}|\n", item.raw_name, pPeer->billboard.id),
                to_char(pPeer->billboard.show),
                to_char(pPeer->billboard.isBuying),
                pPeer->billboard.price,
                to_char(pPeer->billboard.perItem),
                to_char(!pPeer->billboard.perItem)
            )
        });
    }
    else if (const auto it = rml_pages.find(hPipe["buttonClicked"]); it != rml_pages.end())
    {
        send_varlist(event.peer, { "OnDialogRequestRML", it->second });
    }
    else if (hPipe["buttonClicked"] == "trade")
    {
        ::peer peer{.user_id = atoi(hPipe["netID"].c_str())};

        peer.growid = peer.mysql_select<std::string>("growid", "", true);

        send_varlist(event.peer, { "OnStartTrade", peer.growid, peer.user_id });
    }
}
