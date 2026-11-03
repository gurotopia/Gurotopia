#pragma once

#include <unordered_map>

extern const std::unordered_map<u_char, void(*)(ENetEvent&, ::gamePacket)> gamePacket_pool;
