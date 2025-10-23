#include "pch.hpp"
#include "ageworld.hpp"

#if defined(_MSC_VER)
    using namespace std::chrono;
#else
    using namespace std::chrono::_V2;
#endif
using namespace std::literals::chrono_literals;

void ageworld(ENetEvent& event, const std::string_view text)
{
    auto &peer = _peer[event.peer];

    auto w = worlds.find(peer->recent_worlds.back());
    if (w == worlds.end()) return;

    std::vector<block> &blocks = w->second.blocks;
    for (std::size_t i = 0; i < blocks.size(); ++i)
    {
        block &block = blocks[i];
        ::item &item = items[block.fg];
        if (item.type == type::PROVIDER || item.type == type::SEED) // @todo
        {
            block.tick -= 86400s;
            tile_update(event, 
            {
                .id = block.fg, 
                .punch = { (int)i % 100, (int)i / 100 }
            }, block, w->second);
        }
    }
    packet::create(*event.peer, false, 0, { "OnConsoleMessage", "aged world by `w1 day``." });
}