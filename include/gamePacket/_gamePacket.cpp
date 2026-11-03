#include "pch.hpp"

#include "movement.hpp"
#include "tile_change.hpp"
#include "tile_activate.hpp"
#include "item_activate.hpp"
#include "item_activate_object.hpp"
#include "ping_reply.hpp"
#include "disconnect.hpp"
#include "set_icon_state.hpp"

#include "_gamePacket.hpp"

const std::unordered_map<u_char, void(*)(ENetEvent&, ::gamePacket)> gamePacket_pool
{
    {0x00, &movement},
    {0x03, &tile_change},
    {0x07, &tile_activate},
    {0x0a, &item_activate},
    {0x0b, &item_activate_object},
    {0x12, &set_icon_state},
    {0x15, &ping_reply},
    {0x1a, &disconnect}
};
