#pragma once
#ifndef EVENT_TYPE_HPP
#define EVENT_TYPE_HPP

    extern std::unordered_map<ENetEventType, std::function<void(ENetEvent&)>> event_pool;

#endif