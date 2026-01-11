#include "pch.hpp"
#include "movement.hpp"
#include "tile_change.hpp"
#include "tile_activate.hpp"
#include "item_activate.hpp"
#include "item_activate_object.hpp"
#include "ping_reply.hpp"
#include "__states.hpp"

std::unordered_map<u_char, std::function<void(ENetEvent&, state)>> state_pool
{
    {0x00, std::bind(&movement, std::placeholders::_1, std::placeholders::_2)},
    {0x03, std::bind(&tile_change, std::placeholders::_1, std::placeholders::_2)},
    {0x07, std::bind(&tile_activate, std::placeholders::_1, std::placeholders::_2)},
    {0x0a, std::bind(&item_activate, std::placeholders::_1, std::placeholders::_2)},
    {0x0b, std::bind(&item_activate_object, std::placeholders::_1, std::placeholders::_2)},
    {0x15, std::bind(&ping_reply, std::placeholders::_1, std::placeholders::_2)}
};