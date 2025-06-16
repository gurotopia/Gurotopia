#include "pch.hpp"
#include "equip.hpp"

void equip(ENetEvent event, state state)
{
    item &item = items[state.id];
    float &type = _peer[event.peer]->clothing[item.cloth_type];
    
    if (item.cloth_type != clothing::none) 
    {
        /* checks if clothing is already equipped. if so unequip. else equip. */
        type = (type == state.id) ? 0 : state.id;

        clothing_visuals(event);
    }
    else {
        // @todo 100 world locks -> 1 diamond lock
    }
}
