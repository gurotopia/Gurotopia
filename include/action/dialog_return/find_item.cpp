#include "pch.hpp"

#include "tools/string.hpp"

#include "find_item.hpp"

void find_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    std::string id = readch(std::move(pipes[5zu]), '_')[1]; // @note after searchableItemListButton
    
    _peer[event.peer]->emplace(slot(atoi(id.c_str()), 200));
    inventory_visuals(event);
}