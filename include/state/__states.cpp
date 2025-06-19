#include "pch.hpp"
#include "movement.hpp"
#include "tile_change.hpp"
#include "equip.hpp"
#include "pickup.hpp"
#include "__states.hpp"

std::unordered_map<int, std::function<void(ENetEvent, state)>> state_pool
{
    {0, std::bind(&movement, std::placeholders::_1, std::placeholders::_2)},
    {3, std::bind(&tile_change, std::placeholders::_1, std::placeholders::_2)},
    {10, std::bind(&equip, std::placeholders::_1, std::placeholders::_2)},
    {11, std::bind(&pickup, std::placeholders::_1, std::placeholders::_2)}
};