#pragma once

#include <unordered_map>
#include <functional>

const std::array<std::string_view, 7> cmd_requires_arg{
    "msg", "sb", "me", "warp", "punch", "skin", "weather"
};

extern std::unordered_map<std::string_view, std::function<void(ENetEvent&, const std::string_view)>> cmd_pool;
