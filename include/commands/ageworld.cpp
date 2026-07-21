#include "pch.hpp"
#include "ageworld.hpp"

void ageworld(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    std::vector<::block> &blocks = world->blocks;
    for (std::size_t i = 0ull; i < blocks.size(); ++i)
    {
        ::block &block = blocks[i];
        const ::item &item = id_to_item(block.fg);
        
        if (item.type == type::PROVIDER || item.type == type::SEED) // @todo
        {
            block.tick -= 86400;
            send_tile_update(event, 
            {
                .id = block.fg, 
                .punch = ::pos{ (short)i % 100, (short)i / 100 }
            }, block, *world);
        }
    }
}