#include "pch.hpp"
#include "on/SetClothing.hpp"
#include "commands/punch.hpp"
#include "item_activate.hpp"

void item_activate(ENetEvent& event, state state)
{
    auto &peer = _peer[event.peer];

    ::item &item = items[state.id];
    if (item.cloth_type != clothing::none) 
    {
        float &cloth_type = peer->clothing[item.cloth_type];

        cloth_type = (cloth_type == state.id) ? 0 : state.id;
        peer->punch_effect = get_punch_id(state.id); // @todo

        packet::create(*event.peer, true, 0, { "OnEquipNewItem", state.id });
        on::SetClothing(event); // @todo
    }
    else 
    {
        if (state.id == 242 || state.id == 1796) 
        {
            auto lock = std::ranges::find_if(peer->slots, [&state](::slot &slot) { return slot.id == state.id; });
            if (lock == peer->slots.end()) return;

            if (lock->id == 242 && lock->count >= 100) 
            {
                short nokori = peer->emplace(slot(1796, 1));
                if (nokori == 0) peer->emplace(slot(lock->id, -100)); // @note do not take 100 if dl is already at 200.
            }
            else if (lock->id == 1796 && lock->count >= 1) 
            {
                short nokori = peer->emplace(slot(242, 100));
                short hyaku = 100 - nokori;
                if (hyaku == 100) peer->emplace(slot(1796, -1)); // @note do not take 1 if cannot take 100 more wls
                else peer->emplace(slot(242, -hyaku)); // @note return wls if not 100
            }
            inventory_visuals(event);
        }
    }
}
