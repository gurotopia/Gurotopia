#include "pch.hpp"

#include "tools/string.hpp"
#include "on/SetClothing.hpp"

#include "find_item.hpp"

void find_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    std::string id = readch(pipes[5zu], '_')[1]; // @note after searchableItemListButton
    
    _peer[event.peer]->emplace(slot(atoi(id.c_str()), 200));
    inventory_visuals(event);
    on::SetClothing(event); // @note when a item gets added to inventory, clothing equiped resets..
}