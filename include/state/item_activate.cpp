#include "pch.hpp"
#include "on/SetClothing.hpp"
#include "commands/punch.hpp"
#include "item_activate.hpp"

void item_activate(ENetEvent& event, state state)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);

    ::item &item = items[state.id];
    if (item.cloth_type != clothing::none) 
    {
        float &current_cloth = peer->clothing[item.cloth_type]; // @note ID of the current clothing being changed

        current_cloth = (current_cloth == state.id) ? 0 : state.id;

        peer->punch_effect = 0;
        for (float cloth : peer->clothing)
        {
            u_short punch_id = get_punch_id(static_cast<u_int>(cloth));
            if (punch_id != 0)
                peer->punch_effect = punch_id;
        }
        /* @note this is so we can add the latest punch effect (if any) */
        u_short punch_id = get_punch_id(static_cast<u_int>(current_cloth));
        if (punch_id != 0)
            peer->punch_effect = punch_id;

        packet::create(*event.peer, true, 0, { "OnEquipNewItem", state.id });
        on::SetClothing(*event.peer); // @todo
    }
    else 
    {
        if (state.id == 242 || state.id == 1796) 
        {
            const auto lock = std::ranges::find(peer->slots, state.id, &::slot::id);
            if (lock == peer->slots.end()) return;

            if (lock->id == 242 && lock->count >= 100) 
            {
                const short nokori = modify_item_inventory(event, {1796, 1});
                if (nokori == 0) 
                {
                    modify_item_inventory(event, {lock->id, -100});
                    std::string compressed = "You compressed 100 `2World Lock`` into a `2Diamond Lock``!";
                    packet::create(*event.peer, false, 0, { "OnTalkBubble", peer->netid, compressed.c_str(), 0u, 1u });
                    packet::create(*event.peer, false, 0, { "OnConsoleMessage",          compressed.c_str()         });
                }
            }
            else if (lock->id == 1796 && lock->count >= 1)
            {
                const short nokori = modify_item_inventory(event, {242, 100});
                short hyaku = 100 - nokori;
                if (hyaku == 100) 
                {
                    modify_item_inventory(event, {lock->id, -1});
                    std::string shattered = "You shattered a `2Diamond Lock`` into 100 `2World Lock``!";
                    packet::create(*event.peer, false, 0, { "OnTalkBubble", peer->netid, shattered.c_str(), 0u, 1u });
                    packet::create(*event.peer, false, 0, { "OnConsoleMessage",          shattered.c_str()         });
                }
                else modify_item_inventory(event, ::slot(242, -hyaku)); // @note return wls if can't hold 100
            }
        }
    }
}
