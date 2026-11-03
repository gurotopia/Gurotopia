#pragma once

#include <unordered_map>

extern const std::unordered_map<std::string_view, void(*)(ENetEvent&, std::string_view)> cmd_pool;
