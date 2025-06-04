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
    auto it = std::find_if(ifloats.begin(), ifloats.end(), [&](const ifloat& i) 
    {
        const int ix = std::lround(i.pos[0]);
        const int iy = std::lround(i.pos[1]);
        return (std::abs(ix - x) <= 1) && (std::abs(iy - y) <= 1);
    });

    if (it != ifloats.end()) 
    {
        short remember_count = it->count;
        if (it->id != 112)
        {
            short excess = peer->emplace(slot{it->id, remember_count});
            it->count -= (it->count - excess);
        }
        else it->count = 0; // @todo if gem amount is maxed out, do not take any.
        
        drop_visuals(event, {it->id, it->count}, it->pos, state.id/*@todo*/);
        if (it->count == 0) 
        {
            ifloats.erase(it);
            if (it->id != 112)
            {
                item &item = items[it->id];
                gt_packet(*event.peer, false, 0, {
                    "OnConsoleMessage",
                    std::format("Collected `w{} {}``. Rarity: `w{}``", remember_count, item.raw_name, item.rarity).c_str()
                });
                inventory_visuals(event);
            }
            else 
            {
                peer->gems += remember_count;
                gt_packet(*event.peer, false, 0, {
                    "OnSetBux",
                    peer->gems,
                    1,
                    1
                });
            }
        }
    }
}