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

    u_short left = 0;
    if (object->id != 112/*gem*/)
    {
        const ::item &item = id_to_item(object->id);

        const u_short remember_count = object->count;
        left = pPeer->emplace(::slot(object->id, remember_count)); // @return remains after reaching 200
        const u_short collected = remember_count - left;
        if (collected ==/*unsigned*/ 0) return; // @note backpack full: leave it where it is

        on::ConsoleMessage(event.peer, (item.rarity >= 999) ?
            std::format("Collected `w{} {}``.",                collected, item.raw_name) :
            std::format("Collected `w{} {}``. Rarity: `w{}``", collected, item.raw_name, item.rarity)
        );
    }
    else
    {
        pPeer->gems += object->count;
        pPeer->save();
        on::SetBux(event);
    }
    remove_object(event, object->uid);
    const ::object remember_object = *object;
    world->objects.erase(object); // @note erase before add_object(), which can move the list and break the iterator

    if (left > 0) add_object(event, ::slot(remember_object.id, left), remember_object.pos, *world); // @note the rest stays on the ground
}