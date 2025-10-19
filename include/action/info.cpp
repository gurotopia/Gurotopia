#include "pch.hpp"
#include "tools/string.hpp"
#include "info.hpp"

std::vector<std::string> properties(std::byte property) 
{
    std::vector<std::string> temp{};

    u_char _property = std::to_integer<u_char>(property);
    if (_property == 14) temp.emplace_back("`oA lock makes it so only you (and designated friends) can edit an area.``");

    if (_property & 01) temp.emplace_back("`1This item can be placed in two directions, depending on the direction you're facing.``");
    if (_property & 02) temp.emplace_back("`1This item has special properties you can adjust with the Wrench.``");
    if (_property & 04) temp.emplace_back("`1This item never drops any seeds.``");
    if (_property & 0x08) temp.emplace_back("`1This item can't be destroyed - smashing it will return it to your backpack if you have room!``");
    if (_property & 0x10) temp.emplace_back("`1This item can be transmuted.``");
    if (_property & 0x90) temp.emplace_back("`1This item can only be used in World-Locked worlds.``");

    return temp;
}

void action::info(ENetEvent& event, const std::string& header)
{
    std::string itemID = readch(header, '|')[4];

    item &item = items[atoi(itemID.c_str())];

    auto section = [](const auto& range) 
    {
        if (range.empty()) return std::string{};
        
        std::string list{};
        for (const auto& buff : range) 
            list.append(std::format("add_textbox|{}|left|\n", buff));

        return list;
    };

    packet::create(*event.peer, false, 0,
    {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_ele_icon|big|`wAbout {0}``|left|{1}|{2}|\n"
            "add_spacer|small|\n"
            "add_textbox|{3}|left|\n"
            "add_spacer|small|\n"
            "{4}"
            "{5}"
            "add_spacer|small|\n"
            "embed_data|itemID|{1}\n"
            "end_dialog|continue||OK|\n",
            item.raw_name, item.id, "0"/*@todo*/, item.info, 
            (item.rarity == 999) ? "" : std::format("add_textbox|Rarity: `w{}``|left|\nadd_spacer|small|\n", item.rarity), 
            section(properties(item.property))
        ).c_str()
    });
}