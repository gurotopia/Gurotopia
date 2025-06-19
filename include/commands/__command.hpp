#pragma once
#ifndef __COMMAND_HPP
#define __COMMAND_HPP

    extern std::unordered_map<std::string_view, std::function<void(ENetEvent&, const std::string_view)>> cmd_pool;

#endif