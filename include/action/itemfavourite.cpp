#include "pch.hpp"
#include "on/ConsoleMessage.hpp"

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
        const std::string &message = "You cannot favorite any more items. Remove some from your list and try again.";

        send_varlist(event.peer, { "OnTalkBubble", pPeer->netid, message, 0u, 1u });
        on::ConsoleMessage(event.peer, message);
        return;
    }
    send_varlist(event.peer, { "OnFavItemUpdated", stoi(id), (fav) ? 0 : 1 });
    if (fav) pPeer->fav.erase(it);
    else pPeer->fav.emplace_back(stoi(id));
}
