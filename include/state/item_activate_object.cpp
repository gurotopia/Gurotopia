#include "pch.hpp"
#include "on/SetBux.hpp"
#include "on/ConsoleMessage.hpp"

#include "item_activate_object.hpp"

void item_activate_object(ENetEvent& event, state state) 
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    auto object = std::ranges::find(world->objects, state.id, &::object::uid);

    if (object->id != 112/*gem*/)
    {
        auto item = std::ranges::find(items, object->id, &::item::id);

        u_short remember = object->count;
        object->count = pPeer->emplace(::slot(object->id, object->count)); // @return remains after reaching 200
        if (object->count > 0)
        {
            add_object(event, ::slot(object->id, object->count), object->pos, *world);
        }
        u_short collected = remember - object->count;
        if (collected ==/*unsigned*/ 0) return; // @todo

        on::ConsoleMessage(event.peer, (item->rarity >= 999) ?
            std::format("Collected `w{} {}``.",                collected, item->raw_name) :
            std::format("Collected `w{} {}``. Rarity: `w{}``", collected, item->raw_name, item->rarity)
        );
    }
    else 
    {
        pPeer->gems += object->count;
        object->count = 0;
        on::SetBux(event);
    }
    remove_object(event, object->uid);

    if (object->count == 0) world->objects.erase(object);
}