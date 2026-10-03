#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include "lock_edit.hpp"

void lock_edit(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    if (world->owner != pPeer->user_id) return; // @note only the owner can change the lock

    ::pos pos{};
    pos.x = atoi(hPipe["tilex"].c_str());
    pos.y = atoi(hPipe["tiley"].c_str());
    if (pos.x < 0 || pos.x >= 100 || pos.y < 0 || cord(pos.x, pos.y) >= world->blocks.size()) return;
    if (id_to_item(world->blocks[cord(pos.x, pos.y)].fg).type != type::LOCK) return;
    
    world->is_public = atoi(hPipe["checkbox_public"].c_str());
    if (atoi(hPipe["checkbox_disable_music"].c_str()) != 0)
        world->lock_state |= DISABLE_MUSIC;
    else world->lock_state &= ~DISABLE_MUSIC;
    world->minimum_entry_level = atoi(hPipe["minimum_entry_level"].c_str());

    ::block &block = world->blocks[cord(pos.x, pos.y)];

    if (world->is_public) 
         block.state[2] |= S_PUBLIC;
    else block.state[2] &= ~S_PUBLIC;
    on::ConsoleMessage(event.peer, std::format("`2{}`` has set the `$World Lock`` to {}``", pPeer->growid, (block.state[2] & S_PUBLIC) ? "`$PUBLIC" : "`4PRIVATE"));

    send_tile_update(event, {
        .id = block.fg,
        .punch = pos
    }, block, *world);
}