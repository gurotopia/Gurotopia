#include "pch.hpp"
#include "on/SetBux.hpp"
#include "pickup.hpp"

#include <cmath>

void pickup(ENetEvent& event, state state) 
{
    auto &peer = _peer[event.peer];

    auto w = worlds.find(peer->recent_worlds.back());
    if (w == worlds.end()) return;

    auto &ifloats = w->second.ifloats;
    auto f = ifloats.find(state.id);
    if (f == ifloats.end()) return;

    item &item = items[f->second.id];
    if (item.type != std::byte{ GEM })
    {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            (item.rarity >= 999) ?
                std::format("Collected `w{} {}``.", f->second.count, item.raw_name).c_str() :
                std::format("Collected `w{} {}``. Rarity: `w{}``", f->second.count, item.raw_name, item.rarity).c_str()
        });
        short nokori = peer->emplace(slot{f->second.id, f->second.count});
        f->second.count = nokori;
    }
    else 
    {
        peer->gems += f->second.count;
        f->second.count = 0;
        on::SetBux(event);
    }
    drop_visuals(event, {f->second.id, f->second.count}, f->second.pos, state.id/*@todo*/);
    inventory_visuals(event); // @todo confused here... (if I put this higher it duplicates the item.)
    if (f->second.count == 0) ifloats.erase(f);
}