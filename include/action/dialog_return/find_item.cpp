#include "pch.hpp"

#include "tools/string.hpp"
#include "on/SetClothing.hpp"

#include "find_item.hpp"

void find_item(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    for (std::size_t i = 0; i < pipes.size(); ++i) 
    {
        if (pipes[i].contains("searchableItemListButton"))
        {
            std::string id = readch(pipes[i], '_')[1]; // e.g. searchableItemListButton_2_0_-1
            modify_item_inventory(event, ::slot(atoi(id.c_str()), 200));
        } 
    }
}