#include "pch.hpp"
#include "tools/string.hpp"
#include "tools/create_dialog.hpp"
#include "info.hpp"

std::vector<std::string> properties(u_char property) 
{
    std::vector<std::string> temp{};

    if (property == 0xe) temp.emplace_back("`oA lock makes it so only you (and designated friends) can edit an area.``");

    if (property & 01) temp.emplace_back("`1This item can be placed in two directions, depending on the direction you're facing.``");
    if (property & 02) temp.emplace_back("`1This item has special properties you can adjust with the Wrench.``");
    if (property & 04) temp.emplace_back("`1This item never drops any seeds.``");
    if (property & 0x08) temp.emplace_back("`1This item can't be destroyed - smashing it will return it to your backpack if you have room!``");
    if (property & 0x10) temp.emplace_back("`1This item can be transmuted.``");
    if (property & 0x20/*@todo handle Valentine*/) temp.emplace_back("`1This item can kill zombies during a Pandemic!``");
    if (property & 0x90) temp.emplace_back("`1This item can only be used in World-Locked worlds.``");

    return temp;
}

void action::info(ENetEvent& event, const std::string& header)
{
    std::string itemID = readch(header, '|')[4];

    item &item = items[atoi(itemID.c_str())];

    ::create_dialog create_dialog = 
    ::create_dialog()
        .set_default_color("`o")
        .add_label_with_ele_icon("big", std::format("`wAbout {}``", item.raw_name), item.id, 0)
        .add_spacer("small")
        .add_textbox(item.info)
        .add_spacer("small");

    if (item.rarity < 999)
        create_dialog
            .add_textbox(std::format("Rarity: `w{}``", item.rarity))
            .add_spacer("small");
    
    for (const std::string &prop : properties(item.property)) 
        create_dialog.add_textbox(prop);

    packet::create(*event.peer, false, 0,
    {
        "OnDialogRequest",
        create_dialog
            .add_spacer("small")
            .embed_data("itemID", item.id)
            .end_dialog("continue", "", "OK").c_str()
    });
}