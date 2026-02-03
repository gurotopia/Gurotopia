#include "pch.hpp"
#include "on/SetBux.hpp"
#include "item_activate_object.hpp"

void item_activate_object(ENetEvent& event, state state) 
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    if (!worlds.contains(peer->recent_worlds.back())) return;
    ::world &world = worlds.at(peer->recent_worlds.back());

    auto it = world.objects.find(state.id);
    if (it == world.objects.end()) return;
    ::object &object = it->second;

    ::item &item = items[object.id];
    if (item.type != type::GEM)
    {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            (item.rarity >= 999) ?
                std::format("Collected `w{} {}``.", object.count, item.raw_name).c_str() :
                std::format("Collected `w{} {}``. Rarity: `w{}``", object.count, item.raw_name, item.rarity).c_str()
        });
        object.count = peer->emplace(slot{object.id, object.count});
    }
    else 
    {
        peer->gems += object.count;
        object.count = 0;
        on::SetBux(event);
    }
    item_change_object(event, {object.id, object.count}, object.pos, state.id/*@todo*/);
    if (object.count == 0) world.objects.erase(it);
}