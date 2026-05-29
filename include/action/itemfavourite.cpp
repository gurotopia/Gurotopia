#include "pch.hpp"
#include "tools/string.hpp"
#include "itemfavourite.hpp"

void action::itemfavourite(ENetEvent& event, const std::string& header)
{
    std::string id{readch(header, '|')[4]};
    if (id.empty()) return;

    ::peer *pPeer = static_cast<::peer*>(event.peer->data);
    auto it = std::ranges::find(pPeer->fav, stoi(id));
    bool fav = it != pPeer->fav.end();
    if (pPeer->fav.size() >= 20 && !fav)
    {
        constexpr std::string_view message = "You cannot favorite any more items. Remove some from your list and try again.";
        packet::create(*event.peer, false, 0, { "OnTalkBubble", pPeer->netid, message.data(), 0u, 1u });
        packet::create(*event.peer, false, 0, { "OnConsoleMessage", message.data() });
        return;
    }

    packet::create(*event.peer, false, 0, {
        "OnFavItemUpdated",
        stoi(id),
        (fav) ? 0 : 1
    });
    if (fav) pPeer->fav.erase(it);
    else pPeer->fav.emplace_back(stoi(id));
}
