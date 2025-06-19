#pragma once
#ifndef __STATES_HPP
#define __STATES_HPP

    extern std::unordered_map<int, std::function<void(ENetEvent, state)>> state_pool;

#endif