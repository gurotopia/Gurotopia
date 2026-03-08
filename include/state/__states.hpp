#pragma once

#include <unordered_map>

extern std::unordered_map<u_char, std::function<void(ENetEvent&, state)>> state_pool;
