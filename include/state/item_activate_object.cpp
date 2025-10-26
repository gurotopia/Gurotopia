#include "pch.hpp"
#include "on/SetBux.hpp"
#include "item_activate_object.hpp"

#include <cmath>

void item_activate_object(ENetEvent& event, state state) 
{
    auto &peer = _peer[event.peer];

    auto w = worlds.find(peer->recent_worlds.back());
    if (w == worlds.end()) return;
    ::world &world = w->second;

    auto f = world.ifloats.find(state.id);
    if (f == world.ifloats.end()) return;
    ::ifloat &ifloat = f->second;

    ::item &item = items[ifloat.id];
    if (item.type != type::GEM)
    {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            (item.rarity >= 999) ?
                std::format("Collected `w{} {}``.", ifloat.count, item.raw_name).c_str() :
                std::format("Collected `w{} {}``. Rarity: `w{}``", ifloat.count, item.raw_name, item.rarity).c_str()
        });
        ifloat.count = peer->emplace(slot{ifloat.id, ifloat.count});
    }
    else 
    {
        peer->gems += ifloat.count;
        ifloat.count = 0;
        on::SetBux(event);
    }
    item_change_object(event, {ifloat.id, ifloat.count}, ifloat.pos, state.id/*@todo*/);
    if (ifloat.count == 0) world.ifloats.erase(f);
}