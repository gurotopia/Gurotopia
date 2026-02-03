#include "pch.hpp"

#include "lock_edit.hpp"

void lock_edit(ENetEvent& event, const std::vector<std::string> &&pipes)
{
    ::peer *peer = static_cast<::peer*>(event.peer->data);
    auto it = worlds.find(peer->recent_worlds.back());
    if (it == worlds.end()) return;

    ::pos pos{0,0};
    for (u_short i = 0; const std::string &pipe : pipes)
    {
        if      (pipe == "checkbox_public") it->second.is_public = atoi(pipes[i+1].c_str());
        else if (pipe == "tilex")           pos.x = atoi(pipes[i+1].c_str());
        else if (pipe == "tiley")           pos.y = atoi(pipes[i+1].c_str());
        ++i;
    }
    ::block &block = it->second.blocks[cord(pos.x, pos.y)];

    if (it->second.is_public) 
    {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            std::format("`2{}`` has set the `$World Lock`` to `$PUBLIC", peer->ltoken[0]).c_str()
        });
        block.state3 |= S_PUBLIC;
    }
    else {
        packet::create(*event.peer, false, 0, {
            "OnConsoleMessage",
            std::format("`2{}`` has set the `$World Lock`` to `4PRIVATE``", peer->ltoken[0]).c_str()
        });
        block.state3 &= ~S_PUBLIC;
    }

    send_tile_update(event, {
        .id = block.fg,
        .punch = pos
    }, block, it->second);
}