#include "pch.hpp"
#include "onVariant/SetBux.hpp"
#include "onVariant/ConsoleMessage.hpp"

#include "item_activate_object.hpp"

void item_activate_object(ENetEvent& event, ::gamePacket gamePacket) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    auto object = std::ranges::find(world->objects, gamePacket.id, &::object::uid);
    if (object == world->objects.end()) return; // @note already picked up (the game can ask twice)

    const u_int   uid = object->uid;
    const u_short id  = object->id;
    const ::pos   pos = object->pos;
    u_short left = 0;

    if (id != 112/*gem*/)
    {
        const ::item &item = id_to_item(id);

        const u_short remember = object->count;
        left = pPeer->emplace(::slot(static_cast<short>(id), static_cast<short>(remember))); // @return remains after reaching 200
        const u_short collected = remember - left;
        if (collected ==/*unsigned*/ 0) return; // @note backpack full: leave it where it is

        on::ConsoleMessage(event.peer, (item.rarity >= 999) ?
            std::format("Collected `w{} {}``.",                collected, item.raw_name) :
            std::format("Collected `w{} {}``. Rarity: `w{}``", collected, item.raw_name, item.rarity)
        );
    }
    else 
    {
        pPeer->gems += object->count;
        on::SetBux(event);
    }
    remove_object(event, uid);
    world->objects.erase(object); // @note erase before add_object(), which can move the list and break the iterator

    if (left > 0) add_object(event, ::slot(static_cast<short>(id), static_cast<short>(left)), pos, *world); // @note the rest stays on the ground
}