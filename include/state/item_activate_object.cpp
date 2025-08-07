#include "pch.hpp"
#include "on/SetBux.hpp"
#include "item_activate_object.hpp"

#include <cmath>

void item_activate_object(ENetEvent& event, state state) 
{
    auto &peer = _peer[event.peer];

    auto w = worlds.find(peer->recent_worlds.back());
    if (w == worlds.end()) return;

    auto f = w->second.ifloats.find(state.id);
    if (f == w->second.ifloats.end()) return;

    item &item = items[f->second.id];
    if (item.type != std::byte{ GEM })
    {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            (item.rarity >= 999) ?
                std::format("Collected `w{} {}``.", f->second.count, item.raw_name).c_str() :
                std::format("Collected `w{} {}``. Rarity: `w{}``", f->second.count, item.raw_name, item.rarity).c_str()
        });
        f->second.count = peer->emplace(slot{f->second.id, f->second.count});
    }
    else 
    {
        peer->gems += f->second.count;
        f->second.count = 0;
        on::SetBux(event);
    }
    item_change_object(event, {f->second.id, f->second.count}, f->second.pos, state.id/*@todo*/);
    if (f->second.count == 0) w->second.ifloats.erase(f);
}