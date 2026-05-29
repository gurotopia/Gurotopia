#pragma once

#include <unordered_map>

extern std::unordered_map<std::string_view, std::function<void(ENetEvent&, const std::string_view)>> cmd_pool;
