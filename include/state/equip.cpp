#include "database/items.hpp"
#include "database/peer.hpp"
#include "database/world.hpp"
#include "equip.hpp"

void equip(ENetEvent event, state state)
{
    auto& peer = _peer[event.peer];
    auto& item = items[state.id];
    
    /* checks if item is clothing. */
    if (item.cloth_type != clothing::none) {

        /* checks if clothing is already equipped. if so unequip. else equip. */
        if (peer->clothing[item.cloth_type] == state.id)
            peer->clothing[item.cloth_type] = 0;
        else
            peer->clothing[item.cloth_type] = state.id;

        clothing_visuals(event);
    }
    else {
        // @todo 100 world locks -> 1 diamond lock
    }
}
