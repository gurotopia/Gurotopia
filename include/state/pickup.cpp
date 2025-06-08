#include "pch.hpp"
#include "database/items.hpp"
#include "database/peer.hpp"
#include "database/world.hpp"
#include "network/packet.hpp"
#include "pickup.hpp"

#include <cmath>

void pickup(ENetEvent event, state state) 
{
    auto &peer = _peer[event.peer];
    std::vector<ifloat>& ifloats{worlds[peer->recent_worlds.back()].ifloats};
    const int x = std::lround(peer->pos[0]);
    const int y = std::lround(peer->pos[1]);

    auto it = std::find_if(ifloats.begin(), ifloats.end(), [x, y](const ifloat& i) {
        return (std::abs(std::lround(i.pos[0]) - x) <= 1) && 
               (std::abs(std::lround(i.pos[1]) - y) <= 1);
    });

    if (it != ifloats.end()) 
    {
        item &item = items[it->id];
        if (item.type != std::byte{ GEM })
        {
            gt_packet(*event.peer, false, 0, {
                "OnConsoleMessage",
                (item.rarity >= 999) ?
                    std::format("Collected `w{} {}``.", it->count, item.raw_name).c_str() :
                    std::format("Collected `w{} {}``. Rarity: `w{}``", it->count, item.raw_name, item.rarity).c_str()
            });
            short excess = peer->emplace(slot{it->id, it->count});
            it->count = excess;
            inventory_visuals(event);
        }
        else 
        {
            peer->gems += it->count;
            it->count = 0;
            gt_packet(*event.peer, false, 0, {
                "OnSetBux",
                peer->gems,
                1,
                1
            });
        }
        drop_visuals(event, {it->id, it->count}, it->pos, state.id/*@todo*/);
        if (it->count == 0) ifloats.erase(it);
    }
}