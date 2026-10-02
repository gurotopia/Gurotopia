#include "pch.hpp"
#include "legendary.hpp"
#include "find.hpp"

static std::string lower_of(std::string s)
{
    for (char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

/* normal players: the server picks the results, so wands and legendary rewards never show up */
void find_players_list(ENetEvent &event, std::string query)
{
    for (char &ch : query) if (ch == '|' || ch == '\n') ch = ' ';
    while (!query.empty() && std::isspace(static_cast<unsigned char>(query.back()))) query.pop_back();
    const std::string want = lower_of(query);

    std::string list{};
    int shown = 0;
    if (want.size() >= 2)
        for (const ::item &it : items)
        {
            if (it.id == 0 || it.raw_name.empty() || it.id % 2 == 1) continue; // @note seeds are the odd ids
            if (staff_only_item(it.id)) continue;
            if (lower_of(it.raw_name).find(want) == std::string::npos) continue;
            std::string name = it.raw_name;
            for (char &ch : name) if (ch == '|') ch = ' ';
            list += std::format("add_button_with_icon|searchableItemListButton_{}_0_-1|{}|staticBlueFrame|{}||\n", it.id, name, it.id);
            if (++shown >= 50) break;
        }

    std::string d = std::format(
        "set_default_color|`o\n"
        "add_label_with_icon|big|`wFind an Item``|left|1252|\n"
        "add_smalltext|Type part of a name and press Search, then tap an item to choose how many you want.|left|\n"
        "add_text_input|n|Search: |{}|26|\n"
        "add_button|search|Search|noflags|0|0|\n", query);
    if (!list.empty()) d += "add_spacer|small|\n" + list + "add_button_with_icon||END_LIST|noflags|0||\n";
    else if (want.size() >= 2) d += "add_textbox|`oNothing found.``|left|\n";
    d += "add_quick_exit|\nend_dialog|find_item|||\n";
    send_varlist(event.peer, { "OnDialogRequest", d });
}

/* /find [text] -> item search; tapping an item opens its card (see devmenu.cpp) */
void find(ENetEvent& event, const std::string_view text)
{
    std::string query{};
    if (const std::size_t sp = text.find(' '); sp != std::string_view::npos) query = std::string(text.substr(sp + 1));
    for (char &ch : query) if (ch == '|') ch = ' ';

    if (static_cast<::peer*>(event.peer->data)->role < MODERATOR) { find_players_list(event, query); return; }

    send_varlist(event.peer, {
        "OnDialogRequest",
        std::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wFind an Item``|left|1252|\n"
            "add_smalltext|Type part of a name, then tap an item to choose how many you want.|left|\n"
            "add_text_input|n|Search: |{}|26|\n"
            "add_searchable_item_list||sourceType:allItems;listType:iconWithCustomLabel;resultLimit:50|n|\n"
            "add_quick_exit|\n"
            "end_dialog|find_item|||", query)
    });
}
