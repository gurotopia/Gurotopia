#pragma once
#ifndef __ACTION_HPP
#define __ACTION_HPP

    extern std::unordered_map<std::string, std::function<void(ENetEvent, const std::string&)>> action_pool;

#endif