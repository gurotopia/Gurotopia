#pragma once
/* returns true if the item was a consumable and has been used up */
extern bool consumable_use(ENetEvent& event, ::world &world, const ::item &item, ::gamePacket &gamePacket);
