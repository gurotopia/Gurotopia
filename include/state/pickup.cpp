#include "pch.hpp"
#include "pickup.hpp"

#include <cmath>

void pickup(ENetEvent& event, state state) 
{
    auto &peer = _peer[event.peer];

    auto w = worlds.find(peer->recent_worlds.back());
    if (w == worlds.end()) return;

    auto &ifloats = w->second.ifloats;

    auto it = ifloats.find(state.id);
    if (it != ifloats.end()) 
    {
        item &item = items[it->second.id];
        if (item.type != std::byte{ GEM })
        {
            gt_packet(*event.peer, false, 0, {
                "OnConsoleMessage",
                (item.rarity >= 999) ?
                    std::format("Collected `w{} {}``.", it->second.count, item.raw_name).c_str() :
                    std::format("Collected `w{} {}``. Rarity: `w{}``", it->second.count, item.raw_name, item.rarity).c_str()
            });
            short excess = peer->emplace(slot{it->second.id, it->second.count});
            it->second.count = excess;
        }
        else 
        {
            peer->gems += it->second.count;
            it->second.count = 0;
            gt_packet(*event.peer, false, 0, {
                "OnSetBux",
                peer->gems,
                1,
                1
            });
        }
        drop_visuals(event, {it->second.id, it->second.count}, it->second.pos, state.id/*@todo*/);
        inventory_visuals(event); // @todo confused here... (if I put this higher it duplicates the item.)
        if (it->second.count == 0) ifloats.erase(it);
    }
}