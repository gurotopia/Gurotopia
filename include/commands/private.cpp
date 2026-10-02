#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "private.hpp"

/* /private        -> toggles the current world between public and private
 * /private 1      -> force private
 * /private 0      -> force public
 */
void private_cmd(ENetEvent& event, const std::string_view text)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    if (pPeer->role < MODERATOR)
    {
        on::ConsoleMessage(event.peer, "`4Only staff can use this.``");
        return;
    }

    if (pPeer->netid == 0)
    {
        on::ConsoleMessage(event.peer, "`4You must be in a world.``");
        return;
    }

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    std::string arg{ text };
    if (const std::size_t sp = arg.find(' '); sp != std::string::npos) arg = arg.substr(sp + 1);
    else arg.clear();

    bool make_private;
    if (arg.empty())
    {
        make_private = static_cast<bool>(world->is_public); // toggle
    }
    else
    {
        std::erase_if(arg, [](char c){ return !std::isdigit(static_cast<unsigned char>(c)); });
        if (arg.empty())
        {
            on::ConsoleMessage(event.peer, "`4Usage: /private or /private {0|1}``");
            return;
        }
        make_private = (std::stoi(arg) != 0);
    }

    world->is_public = !make_private;

    // keep the world lock tile's flag in sync
    for (::block &block : world->blocks)
    {
        if (block.fg == 0) continue;
        if (world->is_public)
             block.state[2] |= S_PUBLIC;
        else block.state[2] &= ~S_PUBLIC;
    }

    on::ConsoleMessage(event.peer,
        std::format("`2`w{}`` is now {}.``",
                    world->name, make_private ? "`4PRIVATE" : "`$PUBLIC"));
}
