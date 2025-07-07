#include "pch.hpp"
#include "equip.hpp"

void equip(ENetEvent& event, state state)
{
    auto &peer = _peer[event.peer];
    item &item = items[state.id];

    if (item.cloth_type != clothing::none) 
    {
        float &cloth_type = peer->clothing[item.cloth_type];
        
        /* checks if clothing is already equipped. if so unequip. else equip. */
        cloth_type = (cloth_type == state.id) ? 0 : state.id;

        clothing_visuals(event);
    }
    else 
    {
        if (state.id == 242 || state.id == 1796) 
        {
            auto wl = std::ranges::find_if(peer->slots, [](::slot &slot) { return slot.id == 242; });
            auto dl = std::ranges::find_if(peer->slots, [](::slot &slot) { return slot.id == 1796; });
            if (wl != peer->slots.end() && wl->count >= 100) 
            {
                u_int hyaku_goto = wl->count / 100;
                short nokori = peer->emplace(slot(1796, hyaku_goto));
                peer->emplace(slot(242, -100 * (hyaku_goto - nokori))); // @note do not take 100 if dl is already at 200.
            }
            else if (dl != peer->slots.end() && dl->count >= 100) 
            {
                u_int hyaku_goto = dl->count / 100;
                short nokori = peer->emplace(slot(7188, hyaku_goto));
                peer->emplace(slot(1796, -100 * (hyaku_goto - nokori))); // @note do not take 100 if bgl is already at 200.
            }
        }
        inventory_visuals(event);
    }
}
