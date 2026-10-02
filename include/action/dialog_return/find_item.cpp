#include "pch.hpp"
#include "commands/devmenu.hpp"

#include "commands/find.hpp"
#include "commands/legendary.hpp"
#include "find_item.hpp"

void find_item(ENetEvent& event, const ::hPipe &hPipe)
{
    const std::string btn = hPipe["buttonClicked"];
    const bool staff = static_cast<::peer*>(event.peer->data)->role >= MODERATOR;
    if (!btn.starts_with("searchableItemListButton_"))
    {
        if (!staff) find_players_list(event, hPipe["n"]); // @note the Search button (or Enter)
        return;
    }
    const std::vector<std::string> parts = readch(btn, '_'); // e.g. searchableItemListButton_2_0_-1
    if (parts.size() < 2 || parts[1].empty()) return;
    const int id = atoi(parts[1].c_str());
    if (!staff && staff_only_item(id)) return;
    find_item_popup(event, id);
}
