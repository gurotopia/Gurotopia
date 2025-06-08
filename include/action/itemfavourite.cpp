#include "pch.hpp"
#include "database/peer.hpp"
#include "network/packet.hpp"
#include "itemfavourite.hpp"

#include "tools/string_view.hpp"

void itemfavourite(ENetEvent event, const std::string& header)
{
    std::string id{readch(std::string{header}, '|')[4]};
    if (id.empty()) return;

    auto &peer = _peer[event.peer];
    auto it = std::ranges::find(peer->fav, stoi(id));
    bool fav = it != peer->fav.end();
    if (peer->fav.size() >= 20 && !fav) return; // @note allows unfav even if there are already 20.

    gt_packet(*event.peer, false, 0, {
        "OnFavItemUpdated",
        stoi(id),
        (fav) ? 0 : 1
    });
    if (fav) peer->fav.erase(it);
    else peer->fav.emplace_back(stoi(id));
}
