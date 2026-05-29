#include "pch.hpp"
#include "on/SetBux.hpp"
#include "item_activate_object.hpp"

void item_activate_object(ENetEvent& event, state state)
{
    ::peer *pPeer = static_cast<::peer*>(event.peer->data);

    auto world = std::ranges::find(worlds, pPeer->recent_worlds.back(), &::world::name);
    if (world == worlds.end()) return;

    auto object = std::ranges::find(world->objects, state.id, &::object::uid);
    if (object == world->objects.end()) return;

    auto item = std::ranges::find(items, object->id, &::item::id);

    if (item->type == type::GEM)
    {
        pPeer->gems += object->count;
        on::SetBux(event);

        item_change_object(event, ::slot(0, 0), object->pos, object->uid);
        world->objects.erase(object);
        return;
    }

    auto inv = std::ranges::find(pPeer->slots, object->id, &::slot::id);
    short current = (inv != pPeer->slots.end()) ? inv->count : 0;

    if (current >= 200)
    {
        packet::create(*event.peer, false, 0,
        {
            "OnTextOverlay",
            "`4Inventory Full"
        });
        return;
    }

    short take = std::min<short>(200 - current, object->count);

    modify_item_inventory(event, ::slot(object->id, take));

    packet::create(*event.peer, false, 0,
    {
        "OnConsoleMessage",
        std::format("Collected `w{} {}``.", take, item->raw_name).c_str()
    });

    // IMPORTANT:
    // reduce the EXISTING world object by the amount taken.
    // Do NOT delete and re-drop.
    item_change_object(
        event,
        ::slot(object->id, -take),
        object->pos,
        0
    );
}
