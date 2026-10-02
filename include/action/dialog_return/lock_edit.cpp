#include "pch.hpp"
#include "onVariant/ConsoleMessage.hpp"
#include "database/world.hpp"
#include "commands/lockaccess.hpp"
#include "lock_edit.hpp"

void lock_edit(ENetEvent& event, const ::hPipe &hPipe)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    ::pos pos{};
    pos.x = atoi(hPipe["tilex"].c_str());
    pos.y = atoi(hPipe["tiley"].c_str());

    ::block &block = world->blocks[cord(pos.x, pos.y)];

    /* "Get World Key" button: hand over the tradeable key item instead of
     * toggling anything. only the owner (or staff) may take it. */
    if (hPipe["buttonClicked"] == "getKey")
    {
        if (world->owner != pPeer->user_id && pPeer->role < MODERATOR)
        {
            on::ConsoleMessage(event.peer, "`4Only the world owner can take the World Key.``");
            return;
        }

        modify_item_inventory(event, ::slot(242 /* World Key */, 1));
        on::ConsoleMessage(event.peer,
            std::format("`2You received a `$World Key`` for `w{}``.``", world->name));
        return;
    }

    lock_access_apply(event, *world, hPipe); // @note access list: remove unchecked, add the picked player

    if (atoi(hPipe["checkbox_disable_music"].c_str()) != 0)
        world->lock_state |= DISABLE_MUSIC;
    else world->lock_state &= ~DISABLE_MUSIC;

    world->minimum_entry_level = atoi(hPipe["minimum_entry_level"].c_str());

    /* public/private is staff-only now - it lives behind /private instead */
    const bool wants_public = atoi(hPipe["checkbox_public"].c_str()) != 0;
    if (wants_public != static_cast<bool>(world->is_public))
    {
        if (pPeer->role < MODERATOR)
        {
            on::ConsoleMessage(event.peer,
                "`4Use `$/private`` to change who can enter this world.``");
        }
        else
        {
            world->is_public = wants_public;
            if (world->is_public)
                 block.state[2] |= S_PUBLIC;
            else block.state[2] &= ~S_PUBLIC;
        }
    }

    send_tile_update(event, {
        .id = block.fg,
        .punch = pos
    }, block, *world);
}
